/*!
 * @file
 * calculating value of pi by MC trials
 *
 * command to run:
 * mpirung -n 2 ./bin/pi 100000
 *
 * The command line argument is the total number of trials.
 *
 * benchmarks at the bottom
 * */
#include <random>
#include <stdexcept>

#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/predicates.hpp>
#include <ezl/algorithms/io.hpp>
#include <ezl/algorithms/reduces.hpp>

// to facilitate random number generation
template <class T>
struct RandReal {
public:
  RandReal(T min, T max) : dis{min, max} {
    std::random_device rd;
    gen.seed(rd());
  }
  double operator () () {
    return dis(gen);
  }
private:
  std::mt19937 gen;
  std::uniform_real_distribution<T> dis;
};

void valueOfPi(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout<<"Please provide number of MC trials as argument\n";
    return;
  }
  auto trials = std::stoll(argv[1]);

  RandReal<double> rand01{0.0,1.0};

  ezl::rise(ezl::kick(trials).split())
    .map([&rand01] { 
      auto x = rand01();
      auto y = rand01();
      return x*x + y*y; 
    })
    .filter(ezl::lt(1.))
    .reduce(ezl::count(), 0LL).inprocess()
    .reduce(ezl::sum(), 0LL)
    .map([trials](long long res) { 
      return (4.0 * res / trials); 
    }).colsTransform().dump("", "pi in " + std::to_string(trials) + " trials:")
    .run();
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    valueOfPi(argc, argv);
  } catch (const std::exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}

/*!
 * The benchmarks are using rand() function for random.
 * benchmark results: i7(hdd); input: 4 x 10^9; units: secs
 *  *nprocs* | 1   | 2   | 4    |
 *  ---      |---  |---  |---   |
 *  *time(s)*| 111 | 56  | 39   |
 * 
 * benchmark results: Linux(nfs-3); input: variable; units: secs
 *  *nprocs* | 1x12      | 2x12      | 4x12      | 8x12      |  16x12   |
 *  ---      |---        |---        |---        | ---       |          |
 *  *trials* | 1/8x10^11 | 1/4x10^11 | 1/2x10^11 | 1x10^11   |  2x10^11 |
 *  *time(s)*| 48        | 55        | 58        | 57.5      |  59      |
 */
