/*! 
 * @file
 * demo examples for fromMem, kick, fromFileNames, dumpMem.
 *
 * run with following cmd from project directory to check the results.
 * `mpirun -n 1 ./bin/demoIO`
 *
 * add dump() to any unit to check rows coming out of it.
 * 
 * For more io see `demoFromFile` and `demoRise` as well.
 * */
#include <array>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <vector>

#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/io.hpp>
#include <ezl/algorithms/reduces.hpp>

void demoFromMem(std::string outFile) {
  using std::vector;
  using std::array;
  using std::tuple;
  using std::make_tuple;

  // loads every integer as a row. split() shares the rows among processes.
  // The default is not to split and run the complete data in every process.
  ezl::rise(ezl::fromMem({1, 2, 3}).split())
    .dump(outFile, "fromMem with split")
    .run();

  vector<tuple<int, char>> a;
  a.emplace_back(4, 'c');
  auto buf = ezl::fromMem(a).split();
  auto ld = ezl::rise(buf)
             .dump(outFile, "fromMem from lvalue vector")
             .run();

  // run again
  a.clear(); 
  a.emplace_back(5, 'd');

  ezl::flow(ld).run();

  // run again with rvalue
  buf = buf.buffer(vector<tuple<int, char>>{make_tuple(6,'e')}).split(false);
  ezl::flow(ld).run();

  auto mem = ezl::fromMem(array<float, 5>{{4.1, 2.1, 3.1, 1.1, 0.1}});

  auto flow2 = ezl::rise(mem)
                 .dump(outFile, "load from rvalue array w/o share")
                 .get();
}

void demoIO() {
  using std::vector;
  using std::tuple;
  using std::string;
  using std::to_string;

  // If running multi-process it is better to dump in file rather than stdout
  const std::string outFile = "data/output/demoIO.txt";
  const string inFile = "data/fromFileTests/*.txt";

  demoFromMem(outFile); 

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

  // kick calls the next unit N times with nothing.
  // with split N is split amont the various procs, so that total is N.
  // without split every proc calls the next unit N times.
  ezl::rise(ezl::iota(4).split())
    .filter([] (int i) { return true; }).dump(outFile, "iota 4 total")
    .run();

  ezl::rise(ezl::iota(4,8).split(false))
    .filter([] (int i) { return true; }).dump(outFile, "iota 4 each")
    .run();

  // dump to a file or stdout can be done with `dump` expression which can be
  // added to any unit anywhere in the flow, including rise.
  // first argument to dump is filename, imlies stdout if left blank 
  // second argument is the header information.
  // both the arguments are optional, default to empty string.
  auto s = ezl::dumpMem<string>();
  ezl::rise(ezl::fromFileNames(inFile).split().limitFiles(10))
    .filter(s)
    .run();

  vector<tuple<string>> s2;
  ezl::rise(ezl::fromFileNames(inFile).split())
    .filter(ezl::dumpMem(s2))
    .run();

  // a better way to get the results is get()
  auto rows = ezl::rise(ezl::fromFileNames(inFile).split())
                .get();

  assert((s.buffer().size() == s2.size()) && (s2.size() == rows.size()));
  ezl::Karta::inst().print("number of files: "+to_string(rows.size()));
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    demoIO();
  } catch (const std::exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}
