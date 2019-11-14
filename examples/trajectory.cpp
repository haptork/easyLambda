/*!
 * @file
 * An example of logistic regression training and testing.
 * The data is taken from:
 *
 * command to run:
 * ./bin/trajectory "data/trajectory/traj.txt"
 *
 * For running on some different data-set specify the columns etc. in `fromFile`
 * if needed and give the file(s) as command line argument.
 * */
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include <ezl.hpp>
#include <ezl/algorithms/reduces.hpp>
#include <ezl/algorithms/fromFile.hpp>

using namespace std;

auto difference(const array<float, 3>& prev, const array<float, 3>& next) {
  array<float, 3> diff;
  for(auto i : {0, 1, 2}) {
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

void trajectory(int argc, char* argv[]) {
  const auto epsilon = 0.00001F;
  const string outFile = "data/output/traj.txt";
  std::string inFile = "data/trajectory/traj.txt";
  if (argc > 1) inFile = std::string(argv[1]);

  ezl::rise(ezl::fromFile<array<float, 3>>(inFile)
                .cols({1, 2, 3})
                .colSeparator(" \t"))
      .reduceAll([](const vector<array<float, 3>> & v) {
        return difference(v[0], v[1]);
      }).adjacent(2)
      .reduceAll([](const vector<array<float, 3>> & v) {
        return crossProd(v[0], v[1]);
      }).adjacent(2)
      .map([&epsilon](array<float, 3> prod) {
        array<int, 3> res;
        for (auto i : {0, 1, 2}) {
          res[i] = (fabs(prod[i]) > epsilon) ? (prod[i] / fabs(prod[i])) : 0;
        }
        return res;
      })
      .reduce<2>(ezl::count(), 0).inprocess()
      .reduceAll([](vector<tuple<array<int, 3>, int>> a) {
        sort(a.begin(), a.end());
        return a;
      }).dump(outFile)
      .run(1); // Always runs with single process
}

int main(int argc, char *argv[]) {
  // boost::mpi::environment env(argc, argv, false);
  ezl::Env env{argc, argv, false};
  try {
    trajectory(argc, argv);
  } catch (const exception& ex) {
    cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}
