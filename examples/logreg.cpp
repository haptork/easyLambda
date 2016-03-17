/*!
 * @file
 * An example of logistic regression training and testing.
 * The data is taken from:
 *
 * command to run:
 * mpirung -n 4 ./bin/logreg "data/logreg/train.csv"
 *
 * For running on some different data-set specify the columns etc. in `readFile`
 * Also change the `dim` parameter and inFile variable.
 * Testing data files can be given as arguments after training data file.
 *
 * benchmarks at the bottom
 * */
#include <boost/mpi.hpp>

#include <algorithm>

#include <ezl.hpp>

#include <ezl/algorithms/io.hpp>
#include <ezl/algorithms/reduceAlls.hpp>
#include <ezl/algorithms/reduces.hpp>
#include <ezl/algorithms/readFile.hpp>

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
auto calcGrad(const double &y, const std::array<double, dim> &x,
                const std::array<double, dim> &w) {
  array<double, dim> grad;
  auto dot = 0.;
  for (size_t i = 0; i < w.size(); ++i) {
    dot += w[i] * x[i];
  }
  //auto s = (sigmoid(y * dot) - 1) * y;
  auto s = sigmoid(dot) - y;
  for (size_t i = 0; i < w.size(); ++i) {
    grad[i] = s * x[i];
  }
  return grad;
}

int main(int argc, char* argv[]) {
  boost::mpi::environment env(argc, argv);

  assert(argc > 1 && "Please provide train file(s) glob pattern, followed by "
                     "test file pattern(s).");

  constexpr auto dim = 3;  // number of features
  constexpr auto maxIters = 1000;

  // specify columns and other read properties if required.
  auto reader =
      ezl::readFile<double, array<double, dim>>(argv[1]).colSeparator(",");

  // load once in memory
  auto data = ezl::rise(reader)
                  .runResult();

  if (data.empty()) {
    std::cout<<"no data";
    return 0;
  }

  auto sumArray = [](auto &a, auto &b) -> auto & {
    std::transform(begin(a), end(a), begin(b), begin(b), std::plus<double>());
    return b;
  };

  array<double, dim> w{};  // weights initialised to zero;
  // build flow for final gradient value in all procs
  auto train = ezl::rise(ezl::loadMem(data))
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
    std::transform(begin(w), end(w), begin(grad), begin(wn),
                   [](double a, double b) { return a - gamma * b;});
    norm = calcNorm(wn, w);
    w = std::move(wn);
    constexpr auto epsilon = 0.0001;
    if(norm < epsilon)  break;
  }
  std::cout<<"iterations: "<<iters-1<<std::endl;  // TODO: message
  std::cout<<"norm: "<<norm<<std::endl;
  std::cout<<"final weights: "<<w<<std::endl;
  
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
    std::cout<<"Testing for "<<argv[i]<<std::endl;
    ezl::flow(testFlow).run();
  }
 
  return 0;
}

/*!
 * benchmark results: i7(hdd); input: 450MBs
 *  *nprocs* | 1   | 2   | 4    |
 *  ---      |---  |---  |---   |
 *  *time(s)*| 120 | 63  | 38   |
 * 
 * benchmark results: Linux(nfs-3); input: 2.9GBs; units: secs
 *  *nprocs* | 1x12      | 2x12      | 4x12      | 8x12      |  12x12   |
 *  ---      |---        |---        |---        | ---       |          |
 *  *time(s)*| 190       | 91        | 50        | 36        |  34      |
 */
