/*! 
 * @file
 * demo examples for loadMem, kick, loadFileNames, dump, dumpMem.
 *
 * run with following cmd from project directory to check the results.
 * `mpirun -n 1 ./bin/demoIO`
 *
 * add dump to any unit to check rows coming out of it.
 * 
 * For more io see `demoReadFile` and `demoRise` as well.
 * */
#include <array>
#include <tuple>
#include <vector>

#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/io.hpp>
#include <ezl/algorithms/reduces.hpp>

int demoLoadMem(std::string outFile) {
  using std::vector;
  using std::array;
  using std::tuple;
  using std::make_tuple;

  // loads every integer as a row.
  // split() shares the rows among parallel processes
  ezl::rise(ezl::loadMem({1, 2, 3}).split())
    .dump(outFile, "loadMem with split")
    .run();

  vector<tuple<int, char>> a;
  a.emplace_back(make_tuple(4, 'c'));
  auto buf = ezl::loadMem(a).split();
  auto ld = ezl::rise(buf)
             .dump(outFile, "loadMem from lvalue vector")
             .run();
  a.clear(); 
  a.emplace_back(make_tuple(5, 'd'));

  ezl::flow(ld).run();

  buf.buffer(vector<tuple<int, char>>{make_tuple(6,'e')}).split(false);
  ezl::flow(ld).run();

  auto mem = ezl::loadMem(array<float, 5>{{4.1, 2.1, 3.1, 1.1, 0.1}});

  auto flow2 = ezl::rise(mem)
                 .dump(outFile, "load from rvalue array w/o share")
                 .runResult();

  return 0;
}

int main(int argc, char* argv[]) {
  using std::vector;
  using std::tuple;
  using std::string;

  boost::mpi::environment env(argc, argv);

  // If running multi-process it is better to dump in file rather than stdout
  const string outFile = "";  // to stdout
  const string inFile = "data/readFilesTests/*.txt";

  demoLoadMem(outFile); 

  // kick calls the next unit N times with nothing.
  // with split N is split amont the various procs, so that total is N.
  // without split every proc calls the next unit N times.
  ezl::rise(ezl::kick(40).split())
    .map([] { return 0; })
    .reduce(ezl::count(), 0).inprocess().dump(outFile, "kick 40 total")
    .run();

  ezl::rise(ezl::kick(1).split(false))
    .reduce(ezl::count(), 0).dump(outFile, "kick 1 each")
    .run();

  // dump to a file or stdout can be done with `dump` expression which can be
  // added to any unit anywhere in the flow, including rise.
  // first argument to dump is filename, imlies stdout if left blank 
  // second argument is the header information.
  // both the arguments are optional, default to empty string.
  
  // dumpMem
  auto s = ezl::dumpMem<string>();
  ezl::rise(ezl::loadFileNames(inFile).split())
    .filter(s)
    .run();

  vector<tuple<string>> s2;
  ezl::rise(ezl::loadFileNames(inFile).split())
    .filter(ezl::dumpMem(s2))
    .run();
  
  // a better way to get the results is runResult()
  auto rows = ezl::rise(ezl::loadFileNames(inFile).split())
                .runResult();
  return 0;
}
