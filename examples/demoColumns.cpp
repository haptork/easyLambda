/*
 * @file
 * demo for the column selection.
 *
 * UDF term is used for functions used inside map, filter or other units.
 * 
 * For demonstration the pipelines are just built.
 * Replace .build() with .run() and add .dump() in any unit to check the rows.
 * */
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>

#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/io.hpp>

// to see more on rise and loadMem, see `demoIO`
// the integers in the angular brackets after map <1, 1, 0> is bool mask select
// here first and two cols are selected, third is dropped. It is equivalent to
// <1,2>. 
auto mappy() {
  using std::make_tuple;
  using namespace std::string_literals;
  return ezl::rise(ezl::loadMem({make_tuple(2, 'c', 1.F)}))
           .map<1, 1, 0>([](int, char) { // boolean mask selection
             return "hi"s; 
           });
}

// In the following examples, filter is used just to show the types that
// map is passing on.
void demoColumns() {
  using std::string;
  // colsTransform replaces the input cols selected with UDF resulting cols
  // here 1,2 cols are replaced with string type
  mappy().colsTransform()
      .filter([](string, float) {
        return true;
      }).build();

  // colsResult only passes the cols in result of the map UDF
  // here 1,2 cols are replaced with string type
  mappy().colsResult()
    .filter([](string) {
      return true;
    }).build();

  // cols can be selected by index. Indices are input cols followed by output.
  // here 1,2,3 indices are for input and 4th is for output column
  mappy().cols<4,1>()
    .filter([](string, int) {
      return true;
    }).build();

  // cols can be selected by masking columns with 0s and 1s. 
  // cols are input followed by output.
  // here first and fourth with 1s are selected while 2nd and 3rd are dropped.
  mappy().cols<1,0,0,1>()
    .filter([](int, string) {
      return true;
    }).build();
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    demoColumns();
  } catch (const std::exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}
