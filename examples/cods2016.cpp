/*!
 * @file
 * Demo handling some real data.
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
#include <ezl/algorithms/filters.hpp>
#include <ezl/algorithms/maps.hpp>
#include <ezl/algorithms/fromFile.hpp>
#include <ezl/algorithms/reduces.hpp>
#include <ezl/algorithms/reduceAlls.hpp>

using namespace std;

void cods() {
  auto feat1 =
      ezl::fromFile<float, array<float, 4>, char>(
          "data/datachallenge_cods2016/train.csv")
          .cols({"Salary", "English", "Logical", "Quant", "Domain", "Gender"})
          .colSeparator("\t");

  auto source = ezl::rise(feat1)
                    .reduce(ezl::count(), 0).dump("", "Total rows:").oneUp()
                  .filter<2>(ezl::gtAr<4>(0))
                  .build();

  ezl::flow(source)
    .map<3>([](char gender) { return float(gender == 'M' || gender == 'm'); })
      .colsTransform()
    .map<2, 3>(ezl::mergeAr()).colsTransform()
    .reduceAll(ezl::corr()).dump("", "corr. salary, e,l,q, domain, gender")
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
