/*!
 * @file
 * wordcout: frequency of each word in given text files.
 *
 * command to run:
 * mpirun -n 4 ./bin/wordcout "data/fromFileTests/test?.txt"
 *
 * The command line argument is the glob pattern for desired input files.
 * It can run with any number of processors given with `-n ` mpirun argument.
 *
 * benchmarks at the bottom
 * */
#include <iostream>
#include <stdexcept>
#include <string>
#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/fromFile.hpp>
#include <ezl/algorithms/reduces.hpp>

void wordcount(int argc, char* argv[]) {
  using std::string;
  if (argc < 2) {
    std::cerr << "provide file-glob as argument.\n";
    return;
  }
  const std::string outFile = "data/output/wc.txt";
  ezl::rise(ezl::fromFile<string>(argv[1]).rowSeparator('s').colSeparator(""))
    .reduce<1>(ezl::count(), 0).inprocess()
    .reduce<1>(ezl::sum(), 0).dump(outFile)
    .run();
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    wordcount(argc, argv);
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
 *
 * benchmark results: Linux(lustre); units: secs
 * *nprocs* | 1x24 | 2x24 | 4x24 | 8x24 | 16x24 |
 * ---      |---   |---   |---   | ---  | ---   |
 * *12.5GB* |      |  24  | 16   |  13  |  11   |
 *
 * benchmark results: EC2(nfs-3); input: 3.2GB; units: secs
 *  *nprocs* |    8      |   16      |   32      |
 *  ---      |---        |---        |---        |
 *  *ezl*    | 39        | 21        | 11        |
 *  *PySpark*| 780       | 432       | 294       |
 */
