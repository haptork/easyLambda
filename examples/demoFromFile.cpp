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
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/fromFile.hpp>
#include <ezl/algorithms/reduces.hpp>

void demoFromFile() {
  using std::vector;
  using std::array;
  using std::string;

  const std::string inFile = "data/fromFileTests/test1.txt";
  const std::string inFiles = "data/fromFileTests/test?.txt";
  const std::string lammpsFile = "data/lammps/dump.txt";
  const std::string outFile = "data/output/demoFromFile.txt";

  // all the rows in the files that have columns that can be converted to column
  // types specified in template parameters, are read. By default, the data is
  // equally divided among processes available.  If no properties are set the
  // defaults are used such as '\n' for rowSeparator, " " for columnSeparator,
  // strict schema (rows that are not of same size as columns are ignored, if
  // stictrSchema is set to false then either default ctor / nulls are added
  // for missing columns and extra columns are simply ignored).
  //
  // dump is added for checking the results. When running on multiple processes
  // each process writes its own outFile with process rank prepended.
  ezl::rise(ezl::fromFile<string, array<float, 2>>(inFiles))
      .dump(outFile, "\n -- one")
      .run();

  // shows various properties
  ezl::rise(
      ezl::fromFile<float, int, string>(inFiles)
          .addFileName()        // file name at the end of each row.
          .colSeparator("\t, ") // either of '\t', ',' or ' ' as colSeparator
          .tillEOF()            // Files are shared instead of data in parallel
                                // so each file is fully read by a single process.
          .cols({3, 2, 4})      // column indices to pick from each row
          .limitRows(5)         // limit to n rows from the top.
          .limitFiles(2)        // maximum files to read.
          .strictSchema(false)  // select even if columns are less or more
                                // filling with default value if cols are less.
                                // ignoring cols if are more
      )
      .dump(outFile, "\n -- two")
      .run();

  // demos some properties
  ezl::rise(
      ezl::fromFile<int>(inFile)
          .colSeparator("")  // empty string for whole row as single column.
          .rowSeparator('s') // whiteSpace(\t\n ) as row separator.
          .share(false)      // each process reads all the file(s) data.
      )
      .prll(0.75) // all prll options are valid as with any rise
      .dump(outFile, "\n -- three")
      .run(1.0);  // prll option on run is equally valid

  // Shows how ordered propery can be used to reduce in process with inprocess,
  // This can be efficient if the data in input file(s) is ordered.
  // The rows that have same value of columns selected in ordered property
  // are read by same process if they appear consecutively in an input file.
  // please take note that properties return the modified object.
  auto r5 = ezl::fromFile<int, string>(vector<string>{inFile})
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
      ezl::fromFile<array<double, 3>,int>(lammpsFile)
          .parse(ezl::lammpsSchema()) // inshort xyz.lammps()
          .dropCols({1,2}) // An opposite of cols, a string header list
                           // similar to cols can be given
      )
      .reduce<2>(ezl::count(), 0).inprocess().dump(outFile, "\n-- lammps")
      .run();
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    demoFromFile();
  } catch (const std::exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}
