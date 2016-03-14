/*!
 * @file
 * An example of logistic regression training and testing.
 * The data is taken from:
 *
 * command to run:
 * ./bin/trajectory "data/trajectory/traj.txt"
 *
 * For running on some different data-set specify the columns etc. in `readFile`
 * if needed and give the file(s) as command line argument.
 * */
#include <boost/mpi.hpp>

#include "ezl.hpp"
#include "ezl/algorithms/reduces.hpp"
#include "ezl/algorithms/readFile.hpp"

using namespace std;

auto difference(const array<float, 3>& prev, const array<float, 3>& next) {
  array<float, 3> diff;
  for(auto i : {1, 2, 3}) {
    diff[i] = next[i] - prev[i];
  }
  return diff;
}

auto crossProd(const array<float, 3>& v1, const array<float, 3>& v2) {
  array<float, 3> prod;
  prod[0] = v1[1]*v2[2] - v1[2]*v2[1];
  prod[1] = v1[2]*v2[0] - v1[0]*v2[2];
  prod[2] = v1[0]*v2[1] - v1[1]*v2[0];
  return prod;
}

int main(int argc, char* argv[]) {
  boost::mpi::environment env(argc, argv);
  assert(argc>1 && "provide trajectory file-pattern as argument.");
  const string outFile = "data/output/traj.txt";

  ezl::rise(ezl::readFile<array<float, 3>>(string(argv[1]))
                .cols({1, 2, 3})
                .colSeparator(" \t"))
      .reduceAll([](const vector < array<float, 3> & v) {
        return difference(v[0], v[1]);
      }).adjacent()
      .reduceAll([](const vector < array<float, 3> & v) {
        return crossProd(v[0], v[1]);
      }).adjacent()
      .map([](array<float, 3> prod) {
        array<int, 3> res;
        for (auto i : {1, 2, 3}) {
            res[i] = (x > epsilon) ? it / abs(prod[i]) : 0;
        }
        return res;
      })
      .reduce<2>(ezl::count(), 0).inprocess()
      .reduceAll([](vector<tuple<array<int, 3>, int>> a) {
        sort(a.begin(), a.end());
        return a;
      }).dump(outFile)
      .run(1); // Always run with single process

  return 0;
}

