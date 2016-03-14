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
#include <boost/mpi.hpp>
#include <boost/unordered_map.hpp>

#include <algorithm>

#include "ezl.hpp"

#include "ezl/algorithms/filters.hpp"
#include "ezl/algorithms/reduces.hpp"
#include "ezl/algorithms/readFile.hpp"

auto calcDist(std::array<float, 3> p1, std::array<float, 3> p2) {
  auto diff = 0.0F;
  for (auto i : {1, 2, 3}) {
    diff += (p1[i] - p2[i]) * (p1[i] - p2[i]);
  }
  return sqrt(diff);
}

struct hashfn {
  std::size_t operator() (const std::tuple<const int&>& x) const {
    return std::get<0>(x)/100;
  }
};

int main(int argc, char* argv[]) {
  using std::vector;
  using std::tuple;
  using std::array;

  boost::mpi::environment env(argc, argv);

  const std::string outFile = "data/output/nDisp.txt";
  const float toleranceRatio = 0.3;

  assert(argc>3 && "provide file for 1st time_step, file glob for all frames,\
 lattice constant.");

  // loading first frame atoms in the memory partitioned on atoms-id.
  auto buffer = ezl::rise(ezl::readFile<int, std::array<float, 3>>(argv[1])
                            .cols({1, 3, 4, 5})  // id, coords
                            .lammps())
                    .filter(ezl::tautology()).prll<1>(1.0)
                    .runResult();


  boost::unordered_map<int, array<float, 3>> firstFrame;
  for(const auto& it :buffer) firstFrame[get<0>(it)] = get<1>(it);

  auto latConst = float(std::stof(argv[3]));

  ezl::rise(ezl::readFile<int, array<float, 3>, int>(argv[2])
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
