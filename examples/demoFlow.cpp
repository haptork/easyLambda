/*! 
 * @file
 * demo for data-flow.
 *
 * Shows iterative / cyclic data-flow pipeline, a diamond like pipeline with
 * splitterfollowed by a joiner and other expressions such as addFlow, flow<>()
 * ![figures](../doc/dataflow.png)
 *
 * Results are dumped on stdout.
 * */
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <vector>

#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/io.hpp>
#include <ezl/algorithms/filters.hpp>

/*!
 * returns a map-flow that can be placed in a pipeline later.
 * 
 * In place of auto we can explicitly mention type, the alternate fn. signature: 
 *
 * `shared_ptr<ezl::Flow<tuple<char, int>, tuple<char, string>>> sqr();`
 * 
 * The first tuple has input column types and second has output column types.
 *
 * To the returned flow we can add more units that take the input rows same as
 * this flow streams out or we can add the current flow to the units that
 * stream the rows same as the flow's input row type.
 *
 * We can think of a flow as a black-box stream manipulator that can have 
 * multiple units or flows but is identified with only its input and output
 * types, where one kind of rows stream in to result in another type of stream.
 *
 * If we want to just add more units to a flow and use it only as source, we may
 * also write shared_ptr<ezl::Source<tuple<type1, ...>>>. Similarly, if we only
 * want to use the current flow where data just comes in from some other sources,
 * we may use shared_ptr<ezl::Dest<tuple<type1, ...>>>.
 * */
auto sqr() {
  return ezl::flow<char, int>()
      .map<2>([](int i) { return i * i; }).colsTransform()
      .build();
}

void demoFlow() {
  using std::vector;
  using std::tuple;

  vector<tuple<char, vector<int>>> buf;
  buf.emplace_back('a', vector<int>{2});
  buf.emplace_back('b', vector<int>{3, 4, 5});

  auto pivot = sqr();

  ezl::flow(pivot).run(); // doesn't do anything as there isn't a rise yet

  /*!
   * a circular pipeline, keeps squaring the second column integer until it
   * is greater than 100.
   *
   * The first map also demos that another flow can be executed inside a UDF.
   * As, the flow inside map is a no-op, it's same as returning vector itself.
   * Returning a vector is equivalent to returning multiple rows (unless it is
   * tuple<vector<...>>). So each row streaming in results in multiple rows, For
   * the current input the output rows from map are: (a, 2), (b, 3), (b, 4),
   * (b, 5). Notice how the non-selected input column is present in all the
   * output rows as it should be for a single output row with colsTransform.
   * Next, we add the pivot data flow that receive the output rows from the map and
   * squares them. We add a filter with second column > 100 condition and add a dump
   * to it. Next, we go oneUp from the filter back to pivot and add another filter
   * to it with < 100 condition. Now, we add a circular link back to pivot.
   *
   *                                        |--> | filter (>100) + dump |
   *                                        |                   
   * | rise | --> | map (flatten) | --> | flow (sqr) | --> | filter (<100) |
   *                                        ^                    |
   *                                        |                    |
   *                                        ----<-----------<----|
   * */
  auto ld = ezl::rise(ezl::fromMem(buf).split())
    .map<2>([](const vector<int>& v) {
      return ezl::rise(ezl::fromMem(v)).runResult();
    }).colsTransform()
    .addFlow(pivot)  // adds the flow and continues adding to it
      .filter<2>(ezl::gt(100)).dump()
      .oneUp()  // moves to adding to pivot again
    .filter<2>(ezl::lt(100))
    .addFlow(pivot)
    .run();

  /*!
   * A flow with reduce that returns a vector as a column. To return vector
   * as a column, returns a tuple<vector<>> rather than only vector<> which
   * that implies multiple rows. Instead of returning a new vector everytime
   * it returns a reference and recieves the result as a reference. The normal
   * row column parameters can either be const refs or values, one of the
   * reason being a rows might be streamed to multiple units. The result of
   * a return however is local to the unit, hence it can be recieved with
   * reference parameter, modified and the same reference can be returned.
   * This is far more efficient for big objects like vector etc.
   * The ordered property added is not necessary, however the way we use
   * it in the next data-flow makes ordered a better efficient option to add.
   *
   * when recieving result as ref and returning a ref don't forget to put
   * auto& as explicit return type of your lambda (p.s. I just saw the compile
   * error for that)
   * */
  auto joiner = ezl::flow<int, double>().reduce<1>(
      [](tuple<vector<double>> &ret, int key, double val) -> auto& {
      std::get<0>(ret).emplace_back(val); 
        return ret;
      }, tuple<vector<double>>{}).ordered()
      .build();

  /*!
   * a flow with a splitter followed by a joiner
   * To the rise we add two map units, one that adds a column with half the
   * value and one that adds a column with double the value. To both the values
   * we add joiner data-flow built above that appends the second column of all
   * the rows with the same value of first column type in a single row.
   *
   *    
   * | rise |-->| map (double) |-->| reduce (joiner) |-->| filter(noop) + dump |
   *    |                                  ^
   *    |                                  |
   *    |--> | map (half) |---->------->---|
   *
   * */
  auto source = ezl::fromMem({4, 2, 1, 3, 5}).split();
  auto flow1 = ezl::rise(source)
                  .branchFlow(  // adds flow as a branch
                    ezl::flow<int>()
                      .map([](int x) { return double(x)/2.; })
                      .addFlow(joiner)
                      .build()
                  )
                .map([](int x) { return double(x*2); })
                .addFlow(joiner)
                .filter([](int, vector<double> halfnDouble) {
                   return true; 
                 }).dump("", "number, (half, double)")
                .run();

  // running again with different input
  source = source.buffer({6 ,9 ,8 ,7});
  ezl::flow(flow1).run();
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    demoFlow();
  } catch (const std::exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}
