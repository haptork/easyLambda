/*!
 * @file
 * An example of logistic regression training and testing.
 * The data is taken from:
 *
 * command to run:
 * mpirung -n 4 ./bin/logreg "data/logreg/train.csv"
 *
 * For running on some different data-set specify the columns etc. in `fromFile`
 * Also change the `D` parameter and inFile variable.
 * Testing data files can be given as arguments after training data file.
 *
 * benchmarks at the bottom
 * */
#include <array>
#include <iostream>
#include <stdexcept>
#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/io.hpp>
#include <ezl/algorithms/reduceAlls.hpp>
#include <ezl/algorithms/reduces.hpp>
#include <ezl/algorithms/fromFile.hpp>

using namespace std;
using ezl::fromFile;
using ezl::fromMem;
using ezl::rise;
using ezl::Karta;
using ezl::llmode;
using ezl::flow;

template <size_t D>
double calcNorm(const array<double, D> &weights,
                const array<double, D> &weightsNew) {
  auto sum = 0.;
  for (size_t i = 0; i < weights.size(); ++i) {
    auto minus = weights[i] - weightsNew[i];
    sum += (minus * minus);
  }
  return sqrt(sum);
}

void logreg(int argc, char* argv[]) {
  std::string inFile = "data/logreg/train.csv";
  if (argc < 2) {
    Karta::inst().print0("Provide args as glob pattern for train files followed"
                " by test file pattern(s). Continuing with default: " + inFile);
  } else {
    inFile = std::string(argv[1]);
  }
  constexpr auto D = 3;  // number of features
  constexpr auto maxIters = 1000;
  // specify columns and other read properties if required.
  auto reader = fromFile<double, array<double, D>>(inFile).colSeparator(",");
  auto data = rise(reader).get();  // load once in memory
  if (data.empty()) {
    Karta::inst().print("no data");
    return;
  }
  array<double, D> w{}, wn{}, grad{};  // weights initialised to zero;
  auto sumAr = [](auto &a, auto &b) -> auto & { // updating the reference
    transform(begin(b), end(b), begin(a), begin(a), plus<double>());
    return a;
  };
  auto sigmoid = [](double x) { return 1. / (1. + exp(-x)); };
  auto calcGrad = [&](const double &y, array<double, D> x) {
		auto s = sigmoid(inner_product(begin(w), end(w), begin(x), 0.)) - y;
		for_each(begin(x), end(x), [&s](double& x) { x *= s; });
		return x;
  };
  // build flow for final gradient value in all procs
  auto train = rise(fromMem(data)).map(calcGrad).colsTransform()
                 .reduce(sumAr, array<double, D>{}).inprocess()
                 .reduce(sumAr, array<double, D>{}).prll(1., llmode::dupe)
                 .build();
  auto iters = 0;
  auto norm = 0.;
  while (iters++ < maxIters) {
    tie(grad) =  flow(train).get()[0]; // running flow
    constexpr static auto gamma = 0.002;
    transform(begin(w), end(w), begin(grad), begin(wn),
                   [](double a, double b) { return a - gamma * b;});
    norm = calcNorm(wn, w);
    w = move(wn);
    constexpr auto epsilon = 0.0001;
    if(norm < epsilon)  break;
  }
  Karta::inst().print0("iters: " + to_string(iters-1));
  Karta::inst().print0("norm: " + to_string(norm));

  // building testing flow
  auto testFlow = rise(reader)
                      .map<2>([&](auto x) {
                        auto pred = 0.;
                        for (size_t i = 0; i < get<0>(x).size(); ++i) {
                          pred += w[i] * get<0>(x)[i];
                        }
                        return (sigmoid(pred) > 0.5);
                      }).colsTransform()
                      .reduce<1, 2>(ezl::count(), 0)
                        .dump("", "real-y, predicted-y, count")
                      .build();

  for (int i = 1; i < argc; ++i) { // testing all the file patterns
    reader = reader.filePattern(argv[i]);
    Karta::inst().print0("Testing file " + string(argv[i]));
    flow(testFlow).run();
  }
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    logreg(argc, argv);
  } catch (const exception& ex) {
    cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}

/*!
 * benchmark results: i7(hdd); input: 450MBs
 *  *nprocs* | 1   | 2   | 4    |
 *  ---      |---  |---  |---   |
 *  *time(s)*| 120 | 63  | 38   |
 * 
 * benchmark results: Linux(lustre); input: 48GBs; iterations: 10; units: secs
 *  *nprocs* | 1x24      | 2x24      | 4x24      | 8x24      | 16x24    |
 *  ---      |---        |---        |---        | ---       | ---      |
 *  *time(s)*| 335       | 182       | 98        | 55        | 31       |
 *
 * benchmark results: Linux(lustre); input: weak; iterations: 10; units: secs
 *  *nprocs* | 1x24      | 2x24      | 4x24      | 8x24      | 16x24    |
 *  *input*  | 3GB       | 6GB       | 12GB      | 24GB      | 48GB     |
 *  ---      |---        |---        |---        | ---       | ---      |
 *  *time(s)*| 22        | 22        | 26        | 26        | 29       |
 *  ---      |---        |---        |---        | ---       | ---      |
 *  *nprocs* | 4x12      | 8x6       | 8x12      | 16x6      |
 *  *input*  | 6GB       | 6GB       | 12GB      | 12GB      |
 *  ---      |---        |---        |---        | ---       |
 *  *time(s)*| 23        | 23        | 27        | 26        |
 *
 * benchmark results: Linux(nfs-3); input: 48GBs; iterations: 10; units: secs
 *  *nprocs* | 2x12      | 4x12      | 8x12      | 16x12     |
 *  ---      |---        |---        |---        | ---       |
 *  *time(s)*| 520       | 260       | 140       | 75        |
 *
 * benchmark results: Linux(nfs-3); input: weak; iterations: 10; units: secs
 *  *nprocs* | 2x12      | 4x12      | 8x12      | 16x12     |
 *  *input*  | 3GB       | 6GB       | 12GB      | 24GB      |
 *  ---      |---        |---        |---        | ---       |
 *  *time(s)*| 31        | 31        | 34        | 32 / 90   | 
 *  ---      |---        |---        |---        | ---       |
 *  *nprocs* | 4x6       | 8x6       | 16x6      |
 *  *input*  | 3GB       | 6GB       | 12GB      |
 *  ---      |---        |---        |---        |
 *  *time(s)*| 31        | 31        | 65 / 31   |
 *
 *  **Remark:** the value with 16 x 12 / 16 x 6 sometimes takes long time 
 *              (~90secs / 65secs), usually when benchmark is run for the
 *               first time.
 *
 * benchmark results: Linux(nfs-3); input: 2.9GBs; iterations: 100; units: secs
 *  *nprocs* | 1x12      | 2x12      | 4x12      | 8x12      |  12x12   |
 *  ---      |---        |---        |---        | ---       |          |
 *  *time(s)*| 190       | 91        | 50        | 36        |  34      |
 *
 * benchmark results: EC2(nfs-3); input: 2.2GBs; iterations: 10 ; units: secs
 *  *nprocs*         |    8      |   16      |   32      |
 *  ---              |---        |---        |---        |
 *  *time(s)* ezl    | 54        | 27        | 15        |
 *  *time(s)* PySpark| 150       | 102       | 72        |
 *
 */
