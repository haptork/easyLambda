/*!
 * @file
 * Demo for handling some real data.
 *
 * The data used is from:
 * http://ikdd.acm.org/Site/CoDS2016/datachallenge.html 
 * 
 * To see an example on logistic regression see example `logreg`.
 * */
#include <array>
#include <iostream>
#include <stdexcept>

#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/predicates.hpp>
#include <ezl/algorithms/maps.hpp>
#include <ezl/algorithms/fromFile.hpp>
#include <ezl/algorithms/reduces.hpp>
#include <ezl/algorithms/reduceAlls.hpp>

using namespace std;

void cods() {
  /*!
  * fromFile to read columns from tab separated file.
  * It has comprehensive set of options that can be checked at demoFromFile.
  * */
  auto feat1 =
      ezl::fromFile<float, array<float, 4>, char>(
          "data/datachallenge_cods2016/train.csv")
          .cols({"Salary", "English", "Logical", "Quant", "Domain", "Gender"})
          .colSeparator("\t");
  
  /*!
  * With rise we start a flow, add a reduce to count the rows and get back
  * to rise unit with oneUp. Because of oneUp() the filter we add next is
  * added to rise instead of the reduce. In filter we take second column which
  * is an array of scores and add the filter condition that fourth score must
  * be greater than 0. count and gtAr are generic ezl fn objects.
  *
  * Below is a schematic data-flow. The flow with rise and filter is returned.
  * 
  * | rise | --> | filter (Domain > 0) | -->
  *    |
  *    |-------> | reduce (count) |
  * */ 
  auto source = ezl::rise(feat1)
                    .reduce(ezl::count(), 0).dump("", "Total rows:").oneUp()
                  .filter<2>(ezl::gt<4>(0))
                  .build();

  /*!
  * we add to the source data-flow which is same as adding to last filter
  * unit of it. ColsTransform transforms the selected column in place.
  * The dump is used to display the data on terminal or in file. The
  * first parameter is string file name and second is header string. We
  * leave filename as blank to display the data in terminal.
  * */
  ezl::flow(source)
    .map<3>([](char gender) { return float(gender == 'M' || gender == 'm'); })
      .colsTransform()
    .map<2, 3>(ezl::mergeAr()).colsTransform()
    .reduceAll(ezl::corr<1>())
      .dump("", "correlation of salary with e,l,q, domain, gender")
    .run();
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    cods();
  } catch (const exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}
