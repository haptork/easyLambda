/*! 
 * @file
 * Calculating displaced atoms at each frame from a lammps simulation.
 *
 * default run cmd from project directory:
 * `mpirun -n 1 ./bin/displaced "data/lammps/dump6000.txt" "data/lammps/dump.txt" 3.165
 * 
 * ![displaced data-flow](../doc/displaced.png)
 *
 * benchmarks at the bottom
 * */
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <boost/mpi.hpp>
#include <boost/unordered_map.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/filters.hpp>
#include <ezl/algorithms/reduces.hpp>
#include <ezl/algorithms/fromFile.hpp>

auto calcDist(std::array<float, 3> p1, std::array<float, 3> p2) {
  auto diff = 0.0F;
  for (auto i : {0, 1, 2}) {
    diff += (p1[i] - p2[i]) * (p1[i] - p2[i]);
  }
  return sqrt(diff);
}

// a custom hash function to make partitions spread better across
// many processes.
struct hashfn {
  std::size_t operator() (const std::tuple<const int&>& x) const {
    return std::get<0>(x)/100;
  }
};

void displaced(int argc, char* argv[]) {
  using std::vector;
  using std::tuple;
  using std::array;

  const std::string outFile = "data/output/nDisp.txt";
  const float toleranceRatio = 0.3;

  if (argc < 4) {
    std::cerr << "provide file for 1st time_step, file glob for all frames, "
                 "lattice constant as arguments. See source for defaults.\n";
    return;
  }

  // loading first frame atoms in the memory partitioned on atoms-id.
  auto buffer = ezl::rise(ezl::fromFile<int, std::array<float, 3>, int>(argv[1])
                            .cols({1, 3, 4, 5, 6})  // id, coords
                            .lammps())
                    .filter(ezl::tautology()).prll<1>(1.0)
                    .runResult();

  boost::unordered_map<int, array<float, 3>> firstFrame;
  for(const auto& it :buffer) firstFrame[get<0>(it)] = get<1>(it);

  auto latConst = float(std::stof(argv[3]));

  ezl::rise(ezl::fromFile<int, array<float, 3>, int>(argv[2])
                .cols({1, 3, 4, 5, 6}) // id, coords, timestep
                .lammps())
      .map<1, 2>([&firstFrame](int id, array<float, 3> coords) {
        return calcDist(coords, firstFrame[id]);
      }).prll<1>(1.0).colsTransform()
      .filter<1>(ezl::gt(latConst * toleranceRatio))
      .reduce<2>(ezl::count(), 0).hash<hashfn>().inprocess()
      .reduce<1>(ezl::sum(), 0).hash<hashfn>()
      .reduceAll([](vector<tuple<int, int>> a) {
        sort(a.begin(), a.end());
        return a;
      }).dump(outFile)
      .run();
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    displaced(argc, argv);
  } catch (const std::exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}

/*!
 * benchmark results: i7(hdd); input: 25000 atoms/frame (bcc-50x50) 11-frames
 *  *nprocs* | 1   | 2   | 4    |
 *  ---      |---  |---  |---   |
 *  *time(s)*| 46  | 29  | 19   |
 * 
 * benchmark results: Linux(nfs-3); input: 25000 atoms/frame (bcc-50x50) 22-frames
 *  *nprocs* | 1x12      | 2x12      |
 *  ---      |---        |---        |
 *  *time(s)*| 169       | 249       |
 */
