/*! 
 * @file
 * demo for data-flow.
 *
 * Shows iterative data-flow pipeline, a diamond like pipeline with splitter
 * followed by a joiner and other expressions such as addFlow, flow<>()...
 * ![figures](../doc/dataflow.png)
 *
 * Results are dumped on stdout.
 * */
#include <tuple>
#include <vector>

#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/io.hpp>
#include <ezl/algorithms/filters.hpp>

// returns a map-flow that can be placed in a pipeline later
auto sqr() {
  return ezl::flow<char, int>()
      .map<2>([](int i) { return i * i; }).colsTransform()
      .build();
}

int main(int argc, char* argv[]) {
  using std::vector;
  using std::tuple;
  using std::make_tuple;

  boost::mpi::environment env(argc, argv);

  vector<tuple<char, vector<int>>> buf;
  buf.emplace_back(make_tuple('a', vector<int>{2}));
  buf.emplace_back(make_tuple('b', vector<int>{3, 4, 5}));

  auto pivot = sqr();

  ezl::flow(pivot).run(); // doesn't do anything as there isn't a rise yet

  // a circular pipeline, also showing a pipeline run inside a map.
  auto ld = ezl::rise(ezl::loadMem(buf).split())
    .map<2>([](const vector<int>& v) {
      return ezl::rise(ezl::loadMem(v)).runResult();
    }).colsTransform()
    .addFlow(pivot)  // adds the flow and continues adding to it
      .filter<2>(ezl::gt(100)).dump()
      .oneUp()  // moves to adding to pivot again
    .filter<2>(ezl::lt(100))
    .addFlow(pivot)
    .run();

  // when recieving result as ref and returning a ref don't forget to put
  // auto& as explicit return type of your lambda (I just saw the compile
  // error for that)
  auto joiner = ezl::flow<int, double>().reduce<1>(
      [](int key, double val, tuple<vector<double>> &ret) -> auto& {
      std::get<0>(ret).emplace_back(val); 
        return ret;
      }, tuple<vector<double>>{}).ordered() // w/o ordered output rets reversed
      .build();

  // a pipeline with a splitter followed by a joiner
  auto source = ezl::loadMem({4, 2, 1, 3, 5}).split();
  auto flow1 = ezl::rise(source)
                  .branchFlow(  // adds flow as a branch
                    ezl::flow<int>()
                      .map([](int x) { return double(x)/2.; })
                        .branchFlow(joiner)
                      .build()
                  )
                .map([](int x) { return double(x*2); })
                .addFlow(joiner)
                .filter([](int, vector<double> halfnDouble) {
                   return true; 
                 }).dump("", "number, (half, double)")
                .run();

  // running again
  source = source.buffer({6 ,9 ,8 ,7});

  ezl::flow(flow1).run();

  return 0;
}
