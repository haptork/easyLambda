/*!
 * @file
 * Basic tests for `MPIBridge.hpp`
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#include <tuple>
#include <type_traits>
#include <assert.h>

#include <boost/mpi.hpp>
#include <boost/functional/hash.hpp>

#include <ezl/units/MPIBridge.hpp>
#include <ezl/units/Filter.hpp>
#include <ctorTeller.hpp>

namespace ezl {
namespace test {
using namespace ezl::detail;

void MPIBridgeDataEventTest();

void MPIBridgeTest(int argc, char* argv[]) {
  MPIBridgeDataEventTest();
}

void MPIBridgeDataEventTest() {
  using std::make_tuple;
  using std::make_shared;
  using std::vector;
  using std::array;
  using meta::slct;
  using std::tuple;
  using ezl::ProcReq;
  using ezl::Par;

  using emptyHash = boost::hash<std::tuple<>>;
  auto mb = std::make_shared<MPIBridge<std::tuple<int>, slct<>, emptyHash>>(ProcReq{0}, false, false, emptyHash{});
  auto count = 0;
  auto f1 = [&count]() { ++count; return 0; };
  auto ret = std::make_shared<Filter<decltype(mb)::element_type::otype, slct<>, decltype(f1),slct<1>>>(f1);
  auto pr = Par{vector<int>{0}, array<int, 3>{{1,2,3}}, 0};
  mb->forwardPar(&pr);
  mb->par(pr);
  mb->next(ret, mb);
  mb->dataEvent(make_tuple(3));
  assert(count == 1);
}
}
}
