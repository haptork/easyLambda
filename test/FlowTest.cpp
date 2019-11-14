/*!
 * @file
 * Basic tests for `Reduce.hpp`
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

#include <ezl/units/Filter.hpp>
#include <ezl/pipeline/Flow.hpp>

namespace ezl {
namespace test {
using namespace ezl::detail;

void FlowBasicCallTest();

void FlowTest(int argc, char* argv[]) {
  FlowBasicCallTest();
}

void FlowBasicCallTest() {
  using std::make_tuple;
  using meta::slct;
  using std::tuple;

  auto count = 0;;
  auto f1 = [&count](char c, int i) { 
    ++count; 
    return i+1; 
  };
  auto r1 = std::make_shared<
  Filter<tuple<const int&, const char&, const float&>, slct<2,1>, decltype(f1), slct<1,2,3>>>(f1);

  auto r2 = std::make_shared<
  Filter<tuple<const int&, const char&, const float&>, slct<2,1>, decltype(f1), slct<1,2>>>(f1);

  auto r3 = std::make_shared<
  Filter<tuple<const int&, const char&>, slct<2,1>, decltype(f1), slct<1>>>(f1);

  r1->next(r2, r1);

  auto z1 = std::make_shared<Flow<tuple<const int&, const char&, const float&>,
                                  tuple<const int&, const char&>>>();
  z1->addFirst(r1);
  z1->addLast(r2);
  auto t1 = make_tuple(3, 'c', 4.0F);
  z1->dataEvent(t1);
  assert(count == 2);
  count = 0;
  z1->next(r3, z1);
  z1->dataEvent(t1);
  assert(count == 3);
}
}
}
