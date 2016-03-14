/*! 
 * @file
 * demo for various options for reading file data.
 *
 * For more io see `demoIO` and `demoRise` as well.
 *
 * Results are dumped in outFile, that you can check after running with
 * different number of processes.
 * */
#include <array>
#include <string>
#include <vector>

#include <boost/mpi.hpp>

#include "ezl.hpp"
#include "ezl/algorithms/readFile.hpp"
#include "ezl/algorithms/reduces.hpp"

int main(int argc, char* argv[]) {
  using std::vector;
  using std::array;
  using std::string;

  boost::mpi::environment env(argc, argv);

  const std::string inFile = "data/readFileTests/test1.txt";
  const std::string inFiles = "data/readFileTests/test?.txt";
  const std::string lammpsFile = "data/lammps/dump.txt";
  const std::string outFile = "data/output/demoReadFile.txt";

  // all the rows that have column convertible to column types specified
  // are read. File(s) data is equally divided among processes available.
  // no properties are set to uses defaults such as '\n' for rowSeparator,
  // " " for columnSeparator, strict schema (rows that are not of same size
  // as columns are ignored), runs on all available processes etc.
  // dump is for checking the results, when running on multiple processes
  // each process writes its own outFile with process rank prepended.
  ezl::rise(ezl::readFile<string, array<float, 2>>(inFiles)).dump(outFile, "\n -- one")
    .run();

  // shows various properties like
  ezl::rise(
      ezl::readFile<float, int, string>(inFiles)
          .addFileName()        // file name at the end of each row.
          .colSeparator("\t, ") // either of '\t', ',' or ' ' as colSeparator
          .tillEOF()            // A file is read till end by a process.
          .cols({3, 2, 4})      // column indices to pick from each row
          .top(5)               // limit to n rows from the top.
          .noStrict()           // select even if columns are less or more
                                // filling with default value if cols are less.
      )
      .dump(outFile, "\n -- two")
      .run();

  // demos some properties
  ezl::rise(
      ezl::readFile<int>(inFile)
          .colSeparator("")  // empty string for whole row as single column.
          .rowSeparator('s') // whiteSpace(\t\n ) as row separator.
          .noShare()         // each process reads all the file(s) data.
      )
      .prll(0.75) // all prll options are valid as with any rise
      .dump(outFile, "\n -- three")
      .run(1.0);  // prll option on run is equally valid

  // Shows how ordered propery can be used to reduce in process with inprocess,
  // This can be efficient if the data in input file(s) is ordered.
  // The rows that have same value of columns selected in ordered property
  // are read by same process if they appear consecutively in an input file.
  // please take note that properties return the modified object.
  auto r5 = ezl::readFile<int, string>(vector<string>{inFile})
      .cols({"num", "name"}) // cols based on header
      .ordered<1>();

  ezl::rise(r5)
      .reduce<1>(ezl::count(), 0)
      .inprocess()  // in process reduction
      .dump(outFile, "\n-- four")
      .run();

  // Parse property enables reading non-normalised formats in parallel.
  // here we read lammps files which have atomic/molecular coordinates for
  // every timestep value specified at the top of coordinates (see lammpsFile)
  // The lammpsSchema() appends timestep value to each row. file(s) are read
  // in parallel, a process reads full timestep and in process reduction on
  // timestep can be used.
  ezl::rise(
      ezl::readFile<array<double, 3>,int>(lammpsFile)
          .parse(ezl::lammpsSchema()) // inshort xyz.lammps()
          .dropCols({1,2}) // An opposite of cols, a string header list
                           // similar to cols can be given
      )
      .reduce<2>(ezl::count(), 0).inprocess().dump(outFile, "\n-- lammps")
      .run();

  return 0;
}
