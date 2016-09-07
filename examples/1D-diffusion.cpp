/*!
 * @file
 * 1D-diffusionBench: solution of 1D-heat diffusion equation with explicit finite
 *               difference.
 * Shows various dataflow features written concisely.
 * command to run:
 * mpirun -n 4 ./bin/1D-diffusionBench 100 20
 *
 * */
#include <iostream>
#include <stdexcept>
#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/io.hpp>
#include <ezl/algorithms/partitioners.hpp>
#include <ezl/algorithms/predicates.hpp>
#include <ezl/algorithms/reduces.hpp>

using std::tuple;
using std::vector;
using namespace ezl;

void simulation(int nCells, int nSteps, double leftX, double rightX,
                double sigma, double ao, double coeff) {
  auto dx = (rightX - leftX) / (nCells - 1.);
  auto initTemp = [&](int i) {
    auto x = leftX + dx * i + dx / 2.;
    return ao * exp(-x * x / (2. * sigma * sigma));
  };
  auto stencil = [&](int i, double t) {
    vector<tuple<int, double>> c{
        {i, t}, {i, -2 * coeff * t}, {i - 1, coeff * t}, {i + 1, coeff * t}};
    c.erase(remove_if(begin(c)+1, end(c), lt<1>(1) || gt<1>(nCells-2)), end(c));
    return c;
  };
  auto buf = fromMem(rise(iota(nCells)).map(initTemp).dump("ic").runResult());
  auto fl = rise(buf).map(stencil).colsResult().reduce<1>(sum(), 0.).inprocess()
              .reduce<1>(sum(), 0.).prll(1.).partitionBy(range(nCells)).build();
  for (auto i = 0; i < nSteps; ++i) {
    buf.buffer(flow(fl).runResult());
  }
  rise(buf).dump("final").run();
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    auto cells = 100;
    auto steps = 20;
    if (argc > 2) {
      cells = std::stoi(argv[1]);
      steps = std::stoi(argv[2]);
    }
    simulation(cells, steps, -10., 10., 3., 1., 0.375);
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
 * benchmark results: i7(hdd); ncells: 8000000; steps: 10; units: secs
 *  *nprocs* | 1   | 2   | 4    |
 *  ---      |---  |---  |---   |
 *  *time(s)*| 66  | 36  | 22   |
 * 
 * benchmark results: Linux(nfs-3); ncells: 1e8; steps 10; units: secs
 *  *nprocs* | 1x12      | 2x12      | 4x12      | 8x12      |
 *  ---      |---        |---        |---        | ---       |
 *  *time(s)*| 300       | 156       | 81        | 42        |
 *
 * benchmark results: EC2(nfs-3); ncells: 4e7; steps: 10; units: secs
 *  *nprocs*         |    8      |   16      |   32      |
 *  ---              |---        |---        |---        |
 *  *time(s)* ezl    | 84        | 42        | 25        |
 *  *time(s)* PySpark| 1200      | 588       | 294       |
 *
 */
