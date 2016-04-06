/*! 
 * @file
 * demo for reduceAll.
 *
 * Also see `demoReduce`.
 *
 * For demonstration the pipelines are not built or run.
 * Add .run() at the end of a flow and add .dump() in any unit to check rows.
 * */
#include <array>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <vector>
#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/io.hpp>
#include <ezl/algorithms/reduceAlls.hpp>
#include <ezl/algorithms/filters.hpp>

using namespace std;

// AOS params, vector of tuples value type.
int f(char ch, int i, const vector<tuple<array<float,2>>>& v) {
  return int(v.size());
}

// SOA params, tuple of vectors value type.
// A vector return type to return multiple rows.
// tuple<vector<...>> returns vector as a column type.
auto g(char ch, 
       const vector<int>&, const vector<array<float,2>>& v) {
  return std::vector<int>{int(v.size())};
}


void demoReduceAll() {
  vector<tuple<int, char, array<float, 2>>> inp;
  inp.emplace_back(2, 'c', array<float,2>{{1.F, 2.F}});
  inp.emplace_back(2, 'a', array<float,2>{{2.F, 3.F}});
  inp.emplace_back(4, 'a', array<float,2>{{3.F, 4.F}});
  inp.emplace_back(4, 'c', array<float,2>{{4.F, 5.F}});

  auto pipe1 = ezl::rise(ezl::fromMem(inp)).build();

  // output cols are key, result of the UDF.
  // cols can be selected in any order by specifying indices in cols<...>()
  // Alternatively, cols can be dropped by specifying indices in dropCols<...>()
  ezl::flow(pipe1)
    .reduceAll<2, 1>(f).cols<1>()
    .filter([](int) { 
      return true; 
    });

  // The adjacent(N) expression passes N-1 rows adjacent to each row as vector
  // (having size N). This is useful when the reduction depends on rows after
  // for e.g. central difference, direction of vector from two points etc.
  // There is a similar expression bunch(N), it passes the bunch of N new 
  // rows for reduction every time.
  ezl::flow(pipe1)
    .reduceAll<2>(g).adjacent(2)
    .filter([](char, int) { 
      return true; 
    });

  // gives summary() i.e. count, mean, stddev, min, max of each column
  // cols can be array or non array.
  // With ordered(), the reduction does not wait till end of data to flush the
  // results of a key. It can be used if we know that data coming to a
  // reduceAll is ordered. The reduction is essentially done for one key at a
  // time and the resulting row is flushed as soon as any different key appears.
  // In this way the output remains ordered as well. Although, the ordered expr.
  // does not affect results, it increases speed and sets the result in same 
  // order as input.
  ezl::flow(pipe1)
    .reduceAll<2>(ezl::summary()).ordered()
    .filter([](array<double, 5>) { // count, mean, stddev, max, min
      return true;
    });

  // gives correlation of all value columns agains first value col.
  // cols can be array or non array.
  ezl::flow(pipe1)
    .reduceAll(ezl::corr())
    .filter([](array<double, 4>) { // corr for each column including array cols
      return true;
    });

  // gives hist bins and frequency of all value columns.
  ezl::flow(pipe1)
    .reduceAll<ezl::key<>, ezl::val<1,2>>(ezl::hist(5))
    .filter([](array<double, 2> interval, array<int, 2> colsFrequency) {
      return true;
    });
  // UDF params can be key, vector of value column types, or their const-refs,
  // key, vector of tuple of value column types or their const-refs,
  // tuple of key, tuple of vector of value column types, or their const-refs.
  // It is good to care about const-ref if the size of the object is big.  
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    demoReduceAll();
  } catch (const std::exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}
