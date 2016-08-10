/*!
 * @file
 * 1D-diffusion: solved 1D heat diffusion equation with explicit finite diff.
 *
 * command to run:
 * mpirun -n 4 ./bin/1D-diffusionBench
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

using namespace ezl;

void simulation(int nCells, int nSteps, double leftX = -10.0,
                double rightX = +10.0, double sigma = 3.0, double ao = 1.0,
                double coeff = 0.375) {
  auto dx = (rightX - leftX) / (nCells - 1.);
  auto initTemp = [&](int i) {
    auto x = leftX + dx * i + dx / 2.;
    return ao * exp(-x * x / (2. * sigma * sigma));
  };
  auto interior = [&](const auto &ix) {
    return (std::get<0>(ix) <= 0) || (std::get<0>(ix) >= nCells - 1);
  };
  auto stencil = [&](int i, double t) {
    vector<tuple<int, double>> res{
        {i, t}, {i, -2 * coeff * t}, {i - 1, coeff * t}, {i + 1, coeff * t}};
    res.erase(remove_if(begin(res) + 1, end(res), interior), end(res));
    return res;
  };
  auto buf = fromMem(rise(iota(nCells)).map(initTemp).dump("ic").runResult());
  auto fl = rise(buf).map(stencil).colsResult()
              .reduce<1>(sum(), 0.0).prll(1.).partition(onRange(nCells)).build();
  for (auto i = 0; i < nSteps; ++i) {
    buf.buffer(flow(fl).runResult());
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
    simulation(cells, steps);
  } catch (const std::exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);  
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);  
  }
  return 0;
}