/*! 
 * @file
 * demo for zip.
 *
 * similar to functional paradigm zip with / without key or join in relational
 * database queries.
 *
 * */
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <vector>
#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/predicates.hpp>

void demoZip() {
  using std::vector;
  using std::tuple;
  using std::make_tuple;

  vector<tuple<int, char, float>> inp;
  inp.emplace_back(2, 'd', 1.F);
  inp.emplace_back(4, 'a', 3.F);
  inp.emplace_back(2, 'a', 2.F);
  inp.emplace_back(4, 'd', 4.F);

  // creates a data-flow that outputs rows as is. The split property implies
  // that rows are shared among processes if run in parallel.
  
  auto pipe = ezl::rise(ezl::fromMem(inp).split()).build();
  
  // First, the rows from the pipe are filtered with condition col-1 > 3 and 
  // third column is dropped. Next, the the current output rows are zipped or
  // joined with the output rows of pipe with second column as key. The results
  // are dumped. The output of zip are rows with columns of first input stream
  // followed by columns of second input stream that have the same value in the
  // key column. A rows is streamed ahead as soon as the key streams in from
  // both the inputs. The rows that have no counterpart in the other input
  // stream are not streamed as output. Here the output has 2 rows viz.
  // (4, 'a', 4, 'a', 3.), (4, 'd', 2, 'd', 1.).
  ezl::flow(pipe)
    .filter<1>(ezl::gt(3)).colsDrop<3>()
    .zip<2>(pipe).dump("", "with key").run();

  // The pipe is unlinked from all its sources and destinations.
  pipe->unlink();
  
  // filter that drops first row
  auto isDrop = true;
  auto dropOne = ezl::flow(pipe)
                       .filter<0,0,0>([&isDrop] { 
                         if (isDrop) {
                           isDrop = false; 
                           return false;
                         } 
                         return !isDrop; 
                       }).build();

  // The rows from pipe and dropOne data-flows are zipped without key.
  // Without key the order of the rows decides the zipping / joining.
  // The first row from each of the sources is zipped and so on. The output
  // number of rows is equal to the lesser number of input rows.
  ezl::flow(pipe).zip(dropOne).dump("", "w/o key").run();
}

void demoMultipleSourceZip() {
  using std::tuple; using std::vector;
  vector<tuple<int, char, double>> oddInp;
  oddInp.emplace_back(3, 'a', 3.3);
  oddInp.emplace_back(7, 'c', 7.7);
  oddInp.emplace_back(5, 'b', 5.5);
  vector<tuple<int, char>> evenInp;
  evenInp.emplace_back(8, 'd');
  evenInp.emplace_back(6, 'c');
  evenInp.emplace_back(2, 'a');
  evenInp.emplace_back(4, 'b');

  auto odd = ezl::rise(ezl::fromMem(oddInp)).prll({1})
             .build();
  auto even = ezl::rise(ezl::fromMem(evenInp)).prll({0})
             .build();
  ezl::flow(odd).zip<2>(even).dump("", "odd & even").prll({2}).run();
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    demoZip();
    demoMultipleSourceZip();
  } catch (const std::exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}
