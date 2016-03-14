/*! 
 * @file
 * demo for parallelization options
 * 
 * data and task parallelism in ezl can be controlled with prll option and
 * llmode. Request for procesess needed can be specified as number of 
 * processes, exact ranks of processes or (0.0 - 1.0) ratio of 
 * processes in one level up. This is just a request and allocation is 
 * done according to availablity and pre-allocations. The correctness of
 * the program does not get affected by process requests and number of
 * processes available.
 *
 * ![prll options](../doc/prll.png)
 *
 * a run can have a request for procs needed for e.g. .run(4), .run(0.5)
 * or runResult({1,2,3,0})
 * All the units including rise have a prll option to make such a request.
 * 
 * In addition to allocation request mode can also be specified:
 * - shard: if no key is specified, the processes 
 *          get rows in round-robin manner.
 * - all:   All the processes get all the rows.
 * - none:  Default.
 * - task:  The processes are allocated not relative to the parent processes
 *          but as an independent unit relative to the processs in the run 
 *          expression.
 *
 * defaults:
 * - run: all processes mpirun is launched with.
 * - rise: all processes specified in run.
 * - maps and filters: in-process of their parent unit without any parallelism.
 * - reduce(All): 0.5-0.75 processes of their parent unit without task mode.
 * */

#include <boost/mpi.hpp>

#include "ezl.hpp"
#include "ezl/algorithms/io.hpp"
#include "ezl/algorithms/reduces.hpp"
#include "ezl/algorithms/filters.hpp"

struct hashfn {
  std::size_t operator() (const std::tuple<const int&>& x) const {
    return std::get<0>(x)/100;
  }
};

int main(int argc, char* argv[]) {
  using std::make_tuple;
  using std::tie;

  boost::mpi::environment env(argc, argv);

  auto source = ezl::loadMem({make_tuple(200,'c',1.F)});

  // The example shows a useful idiom.
  // an inprocess reduce followed by another reduce to make parallism much more
  // effective compared to only one reduce. 
  // The filter makes the resulting count available in all processes, finally
  // returned in the variable iSum.
  auto iSum = 0LL;
  tie(iSum) = ezl::rise(source)
                  .reduce(ezl::count(), 0LL)
                    .inprocess()
                  .reduce(ezl::sum(), 0LL)
                  .filter(ezl::tautology())
                    .prll(1., ezl::llmode::all | ezl::llmode::task)
                  .runResult(1.)[0];

  // The source runs in one process, while filter runs on all processes.
  // The rows are distributed to all processes in round-robin fashion.
  ezl::rise(source).prll(1)
    .filter(ezl::tautology())
      .prll(1., ezl::llmode::shard | ezl::llmode::task)
    .run({0, 2, 4});

  // parallelization on filter based on column 1 as key for partitioning the data
  auto flow3 = ezl::rise(source).prll(0.25)
                  .filter(ezl::tautology())
                    .prll<1>(0.75, ezl::llmode::shard | ezl::llmode::task)
                  .reduce<2>(ezl::count(), 0).prll(0.5)
                  .run(4, false);

  // custom partioning function for hashing and partitioning data among processes
  // ordered expression is a way of suggesting that the incoming data is ordered
  // on the key column i.e. rows with same keys are coming together to the unit.
  // The reduce uses this and flushes the results as soon as the key changes
  // rather than waiting till the end of data.
  auto flowOrd = ezl::rise(source)
                   .reduce<1>(ezl::count(), 0).hash<hashfn>().ordered()
                   .build();

  return 0;
}
