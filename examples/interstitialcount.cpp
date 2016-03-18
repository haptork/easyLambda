/*! 
 * @file
 * Calculating interstitials i.e. atoms that are away from ideal lattice in
 * each frame.
 *
 * default run cmd from project directory:
 * `mpirun -n 4 ./bin/interstitialcount "data/lammps/dump*.txt" 3.165 0 bcc
 * */
#include <array>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <vector>
#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/reduces.hpp>
#include <ezl/algorithms/filters.hpp>
#include <ezl/algorithms/readFile.hpp>

#include "AddOffset.cpp"

void icount(int argc, char* argv[]) {
  using namespace ezl;
  using std::vector;
  using std::tuple;

  if (argc < 5) {
    std::cerr << "Please give arguments as fileIn, latConst, origin, bcc/fcc, "
                 "output file tag(optional), check source for defaults.\n";
    return;
  }
  std::string ftag = "";
  if (argc > 5) ftag = std::string{argv[5]};

  const std::string outFile = "data/output/icount" + ftag + ".txt";
  const float toleranceRatio = 0.3;

  auto latConst = float(std::stof(argv[2]));
  auto o = std::stof(argv[3]);
  auto origin = std::array<float, 3> {{o, 0, 0}};
  auto obj = AddOffset(latConst, argv[4], origin);

  // frame, coords
  auto reader = readFile<int, std::array<float, 3>>(argv[1])
                  .lammps().cols({6, 3, 4, 5});

  rise(reader)
      .map<2>(obj)
      .filter<4>(gt(latConst * toleranceRatio))
      .reduce<1>(count(), 0).inprocess().ordered()
      .reduceAll([](vector<tuple<int, int>> a) { // sorting the time-step counts
        sort(a.begin(), a.end());
        return a;
      })
        .dump(outFile)
      .run();
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    icount(argc, argv);
  } catch (const std::exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}
