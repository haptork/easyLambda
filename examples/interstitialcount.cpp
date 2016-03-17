/*! 
 * @file
 * Calculating interstitials i.e. atoms that are away from ideal lattice in
 * each frame.
 *
 * default run cmd from project directory:
 * `mpirun -n 4 ./bin/interstitialcount "data/lammps/dump*.txt" 3.165 0 bcc
 * */
#include <boost/mpi.hpp>
#include <array>

#include <ezl.hpp>
#include <AddOffset.cpp>

#include <ezl/algorithms/reduces.hpp>
#include <ezl/algorithms/filters.hpp>
#include <ezl/algorithms/readFile.hpp>

int main(int argc, char* argv[]) {
  boost::mpi::environment env(argc, argv);
  using namespace ezl;
  using std::vector;
  using std::tuple;

  assert(argc > 4 && "Please give arguments as fileIn, latConst, origin, "
                     "bcc/fcc, tag(optional)");
  std::string ftag = "";
  if (argc>5)
    ftag = std::string{argv[5]};

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

  return 0;
}
