/*! 
 * @file
 * demo for zip.
 *
 * */
#include <array>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <vector>
#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/filters.hpp>

void demoZip() {
  using std::vector;
  using std::array;
  using std::tuple;
  using std::make_tuple;
  using std::tie;
  using namespace std::string_literals;


  vector<tuple<int, char, float>> inp;
  inp.emplace_back(2, 'd', 1.F);
  inp.emplace_back(4, 'a', 3.F);
  inp.emplace_back(2, 'a', 2.F);
  inp.emplace_back(4, 'd', 4.F);

  auto pipe1 = ezl::rise(ezl::fromMem(inp).split()).build();

  ezl::flow(pipe1)
    .filter<1>(ezl::gt(3)).cols<1, 2>()
    .zip<2>(pipe1).dump("", "with key").run();

  auto pipe3 = ezl::rise(ezl::fromMem(inp)).build();

  auto pipe4 = ezl::flow(pipe3)
                 .filter<1>(ezl::lt(3)).cols<1,2>().build();

  ezl::flow(pipe3).zip(pipe4).dump("", "w/o key").run();

}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    demoZip();
  } catch (const std::exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}
