/*!
 * @file
 * An example of logistic regression training and testing.
 * The data is taken from:
 *
 * command to run:
 * mpirung -n 4 ./bin/logreg "data/logreg/train.csv"
 *
 * For running on some different data-set specify the columns etc. in `fromFile`
 * Also change the `dim` parameter and inFile variable.
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

double sigmoid(double x) {
  constexpr auto e = 2.718281828;
  return 1.0 / (1.0 + pow(e, -x));
}

template <size_t dim>
double calcNorm(const array<double, dim> &weights,
                const array<double, dim> &weightsNew) {
  auto sum = 0.;
  for (size_t i = 0; i < weights.size(); ++i) {
    auto minus = weights[i] - weightsNew[i];
    sum += (minus * minus);
  }
  return sqrt(sum);
}

template <size_t dim>
auto calcGrad(const double &y, const array<double, dim> &x,
                const array<double, dim> &w) {
  array<double, dim> grad;
  auto dot = std::inner_product(::begin(w), ::end(w), ::begin(x), 0);
  //auto s = (sigmoid(y * dot) - 1) * y;
  auto s = sigmoid(dot) - y;
  for (size_t i = 0; i < w.size(); ++i) {
    grad[i] = s * x[i];
  }
  return grad;
}

void logreg(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Please provide arguments as glob pattern for train file(s), "
            "followed by test file pattern(s). Check source for defaults or "
            "for running on some other data-format.";
    return;
  }

  constexpr auto dim = 3;  // number of features
  constexpr auto maxIters = 1000;

  // specify columns and other read properties if required.
  auto reader =
      ezl::fromFile<double, array<double, dim>>(argv[1]).colSeparator(",");

  // load once in memory
  auto data = ezl::rise(reader)
                  .runResult();

  if (data.empty()) {
    cout<<"no data";
    return;
  }

  auto sumArray = [](auto &a, auto &b) -> auto & {
    transform(begin(b), end(b), begin(a), begin(a), plus<double>());
    return a;
  };

  array<double, dim> w{};  // weights initialised to zero;
  // build flow for final gradient value in all procs
  auto train = ezl::rise(ezl::fromMem(data))
                   .map([&w](auto& y, auto& x) {
                     return calcGrad(y, x, w);    
                   }).colsTransform()
                   .reduce(sumArray, array<double, dim>{}).inprocess()
                   .reduce(sumArray, array<double, dim>{})
                     .prll(1., ezl::llmode::task | ezl::llmode::all)
                   .build();
                 
  auto iters = 0;
  auto norm = 0.;
  while (iters++ < maxIters) {
    array<double, dim> wn, grad;
    tie(grad) =  ezl::flow(train).runResult()[0]; // running flow
    constexpr static auto gamma = 0.002;
    transform(begin(w), end(w), begin(grad), begin(wn),
                   [](double a, double b) { return a - gamma * b;});
    norm = calcNorm(wn, w);
    w = move(wn);
    constexpr auto epsilon = 0.0001;
    if(norm < epsilon)  break;
  }
  cout<<"iterations: "<<iters-1<<endl;  // TODO: message
  cout<<"norm: "<<norm<<endl;
  cout<<"final weights: "<<w<<endl;
  
  // building testing flow
  auto testFlow = ezl::rise(reader)
                      .map<2>([&w](auto x) {
                        auto pred = 0.;
                        for (size_t i = 0; i < get<0>(x).size(); ++i) {
                          pred += w[i] * get<0>(x)[i];
                        }
                        return (sigmoid(pred) > 0.5);
                      }).colsTransform()
                      .reduce<1, 2>(ezl::count(), 0)
                        .dump("", "real-y, predicted-y, count")
                      .build();

  for (int i = 1; i < argc; ++i) {
    reader = reader.filePattern(argv[i]);
    cout<<"Testing for "<<argv[i]<<endl;
    ezl::flow(testFlow).run();
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
 *
 */
