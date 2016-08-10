/*!
 * @file
 * 1D-diffusion: solved 1D heat diffusion equation with explicit finite diff.
 * Shows various dataflow features used in concise way.
 *
 * command to run:
 * mpirun -n 4 ./bin/1D-diffusionSuccint
 *
 * */
#include <iostream>
#include <stdexcept>
#include <string>
#include <boost/mpi.hpp>

#include <ezl.hpp>
#include <ezl/algorithms/reduces.hpp>
#include <ezl/algorithms/predicates.hpp>
#include <ezl/algorithms/io.hpp>
#include <ezl/algorithms/partitioners.hpp>

using std::make_tuple;
using std::tuple;
using std::vector;
using std::to_string;
using std::multiplies;

using ezl::fromMem;
using ezl::rise;
using ezl::iota;
using ezl::gt;
using ezl::lt;
using ezl::sum;
using ezl::onRange;
using ezl::tautology;
using ezl::Karta;

void simulation(int nCells, int nSteps, double leftX, double rightX,
                double sigma, double ao, double coeff) {
  auto dx = (rightX - leftX) / (nCells - 1.);
  auto initTemp = [&](int i) {
    auto x = leftX + dx * i + dx / 2.;
    return ao * exp(-x * x / (2. * sigma * sigma));
  };
  auto stencil = [&](int i, double t) {
    auto c = rise(fromMem({coeff * t, -2 * coeff * t, coeff * t})).build();
    auto f = rise(iota(i-1, i+2)).filter(gt(0) && lt(nCells-1)).zip(c).build();
    return flow(*f + rise(fromMem({make_tuple(i, t)})).build()).runResult();
  };
  auto buf = fromMem(rise(iota(nCells)).map(initTemp).dump("ic.txt").runResult());
  for (auto i = 0; i < nSteps; ++i) {
    buf.buffer(rise(buf).map(stencil).colsResult()
      .reduce<1>(sum(), 0.).prll(1.).partition(onRange(nCells)).runResult());
  }
  rise(buf).filter(tautology()).dump("final.txt").run();
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