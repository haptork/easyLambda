/*!
 * @file
 * wordcout: frequency of each word in given text files.
 *
 * command to run:
 * mpirun -n 4 ./bin/wordcout "data/readFileTests/test?.txt"
 *
 * The command line argument is the glob pattern for desired input files.
 * It can run with any number of processors given with `-n ` mpirun argument.
 * 
 * benchmarks at the bottom
 * */

#include <string>

#include <boost/mpi.hpp>

#include "ezl.hpp"
#include "ezl/algorithms/readFile.hpp"
#include "ezl/algorithms/reduces.hpp"

int main(int argc, char* argv[]) {
  using std::string;
  using std::vector;
  boost::mpi::environment env(argc, argv);

  assert(argc>1 && "provide file-glob as argument.");
  const std::string outFile = "data/output/wc.txt";

  ezl::rise(
      ezl::readFile<string>(argv[1]).rowSeparator('s').colSeparator(""))
    .reduce<1>(ezl::count(), 0).inprocess()
    .reduce<1>(ezl::sum(), 0).dump(outFile)
    .run();
  return 0;
}

/*!
 * benchmark results: i7(hdd); units: secs
 * *nprocs* | 1    | 2    | 4    |
 * ---      |---   |---   |---   |
 * *600MB*  | 13.4 | 8.7  | 7    |
 * *1200MB* | 27.0 | 15.5 | 14.4 |
 *
 * benchmark results: Linux(nfs-3); units: secs
 * *nprocs* | 1x12 | 2x12 | 4x12 | 8x12 |   
 * ---      |---   |---   |---   | ---  |
 * *12.5GB* | 178  | 114  | 82   |  80  |
 */
