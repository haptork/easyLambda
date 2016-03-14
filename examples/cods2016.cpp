/*!
 * @file
 * Demo handling some real data.
 *
 * The data used is from:
 * http://ikdd.acm.org/Site/CoDS2016/datachallenge.html 
 * 
 * To see an example on logistic regression see example `logreg`.
 * */
#include <boost/mpi.hpp>
#include <boost/mpi.hpp>

#include <algorithm>
#include <map>

#include "ezl.hpp"

#include "ezl/algorithms/filters.hpp"
#include "ezl/algorithms/maps.hpp"
#include "ezl/algorithms/readFile.hpp"
#include "ezl/algorithms/reduces.hpp"
#include "ezl/algorithms/reduceAlls.hpp"

using namespace std;

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv);

  // salary, (e, l, q, domain scores), gender
  auto feat1 = ezl::readFile<float, std::array<float, 4>, char>(
                   "data/datachallenge_cods2016/train.csv")
                   .cols({3, 24, 25, 26, 27, 8})
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

  return 0;
}
