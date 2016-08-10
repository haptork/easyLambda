/*!
 * @file
 * Basic tests for `funcInvoke.hpp`
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#include <string>
#include <tuple>
#include <type_traits>
#include <assert.h>
#include <iostream>

#include <ezl/helper/meta/funcInvoke.hpp>
#include <ctorTeller.hpp>

namespace ezl {
namespace test {
using namespace ezl::detail::meta;

void mapInvokeTest();

void funcInvokeTest() {
  mapInvokeTest();
}

void mapInvokeTest() {
  using std::tuple;
  using std::make_tuple;

  auto f1 = [](tuple<int, char, ctorTeller> t) { return make_tuple(3., 'c'); };
  auto f2 = [](int, char, ctorTeller) { return make_tuple(3., 'c'); };
  auto f3 = [](const tuple<int, char, ctorTeller>& t) { return 3; };
  auto f4 = [](const tuple<const int&, const char&, const ctorTeller&>& t) { 
    return make_tuple(3., 'c'); 
  };
  auto f5 = [](const int&, const char&, const ctorTeller&) { return make_tuple(3., 'c'); };

  // invokeMap
  ctorTeller c("invoke 1", true, false);
  invokeMap(f1, make_tuple(3, 'c', c));  // twice copying
  invokeMap(f2, make_tuple(3, 'c', c));  // twice copying
  invokeMap(f3, make_tuple(3, 'c', c));  // once copying
  invokeMap(f5, make_tuple(3, 'c', c));  // once copying
  // Error in this as tuple is R-value
  //invokeMap(f4, make_tuple(3, 'c', ctorTeller("1: call 4")));
  assert(ctorTeller::copyctor() == 6);
  assert(ctorTeller::movector() == 0);

  int nt = 0; char ch =  'c'; 
  ctorTeller mn("invoke 2", true, false);
  assert(ctorTeller::copyctor() == 0);
  invokeMap(f1, std::tie(nt, ch, mn));  // once copying
  invokeMap(f2, std::tie(nt, ch, mn));  // once copying  
  invokeMap(f3, std::tie(nt, ch, mn));  // once copying
  assert(ctorTeller::copyctor() == 3);
  invokeMap(f4, std::tie(nt, ch, mn));  // no copying
  invokeMap(f5, std::tie(nt, ch, mn));  // no copying
  assert(ctorTeller::copyctor() == 3);
  assert(ctorTeller::movector() == 0);

  invokeMap([]() { return true; }, std::tuple<>{});
  invokeMap([](int) { return true; }, std::make_tuple(4));
  invokeMap([](std::tuple<int>) { return true; }, std::make_tuple(4));

  // gives a good error msg
  //invokeMap([](int) { return true; }, std::tuple<>{});
}
}
}
