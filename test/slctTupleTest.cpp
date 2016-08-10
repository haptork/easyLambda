/*!
 * @file
 * Basic tests for `slctTuple.hpp`
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#include <iostream>
#include <tuple>
#include <type_traits>
#include <assert.h>

#include <ezl/helper/meta/slctTuple.hpp>
#include <ezl/helper/meta/slct.hpp>

#include <ctorTeller.hpp>

namespace ezl {
namespace test {
using namespace ezl::detail::meta;

void slctTupleTest() {
  using std::tuple;
  using std::make_tuple;

  // Tuple concatenate type
  static_assert(
      std::is_same<
          tuple<const int&, const char&, const int&>,
          TupleTieType<tuple<int, char>, tuple<int>>>::value,
      "");
  static_assert(
      std::is_same<tuple<const int&, const char&>, TupleTieType<tuple<int>, tuple<char>>>::value,
      "");
  static_assert(std::is_same<tuple<>, TupleTieType<tuple<>, tuple<>>>::value,
                "");
  static_assert(
      !std::is_same<tuple<int, char>, TupleTieType<tuple<int>, tuple<char>>>::value,
      "");
  static_assert(
      !std::is_same<tuple<int&, char&>, TupleTieType<tuple<int>, tuple<char>>>::value,
      "");

  // Tuple concatenate type
  static_assert(
      std::is_same<
          tuple<int, char, int, float>,
          TupleCatType<tuple<int, char>, tuple<int>, tuple<float>>>::value,
      "");
  static_assert(
      std::is_same<tuple<int, char>, TupleCatType<tuple<int, char>>>::value,
      "");
  static_assert(std::is_same<tuple<>, TupleCatType<tuple<>, tuple<>>>::value,
                "");
  static_assert(
      !std::is_same<tuple<int&, char&>, TupleCatType<tuple<int, char>>>::value,
      "");
  static_assert(
      !std::is_same<tuple<const int&, const char&>, TupleCatType<tuple<int, char>>>::value,
      "");
  // no nice msg for these.
  // (Sanity check for sequence takes car of static_error msgs)
  /*
  static_assert(
      std::is_same<tuple<int, int, char>,
                   typename SlctTupleType<tuple<char, int>,
                                          slct<2, 0, 1>>::type>::value,
      "");

  static_assert(
      std::is_same<tuple<int, int, char>,
                   typename SlctTupleType<tuple<char, int>,
                                          slct<2, 5, 1>>::type>::value,
      "");
 */ 
  static_assert(
      std::is_same<tuple<int>,
                   typename SlctTupleType<tuple<int, char>,
                                          slct<1>>::type>::value,
      "");
  static_assert(
      std::is_same<tuple<float, char>,
                   typename SlctTupleType<tuple<int, char, float>,
                                          slct<3, 2>>::type>::value,
      "");

  // SlctTuple
  assert(make_tuple(4., 'c') == slctTuple(make_tuple(2,'c',4.), slct<3, 2>{}));
  assert(make_tuple(2) == slctTuple(make_tuple(2,'c',4.), slct<1>{}));
  assert(tuple<>{} == slctTuple(make_tuple(2,'c',4.), slct<>{}));

  // SlctTuple
  assert(make_tuple(4., 'c') == slctTupleRef(make_tuple(2,'c',4.), slct<3, 2>{}));
  assert(make_tuple(2) == slctTupleRef(make_tuple(2,'c',4.), slct<1>{}));
  assert(tuple<>{} == slctTupleRef(make_tuple(2,'c',4.), slct<>{}));
  // SlctTuple
  int nt = 2; char ch = 'c'; ctorTeller mn1("tupSlct", true, false);
  auto t1 = make_tuple(nt, ch, mn1);
  
  assert(ctorTeller::copyctor() == 1);
  slctTupleRef(t1, slct<3, 2>{});
  assert(ctorTeller::copyctor() == 1);

  slctTuple(t1, slct<1, 3>{});  // copy once
  slctTuple(t1, slct<1>{});
  slctTuple(t1, slct<>{});
  assert(ctorTeller::copyctor() == 2);

  slctTupleRef(t1, slct<1, 3>{});
  slctTupleRef(t1, slct<1>{});
  slctTupleRef(t1, slct<>{});
  assert(ctorTeller::copyctor() == 2);

  static_assert(colCount<std::tuple<int, char>>::value == 2, "");
  static_assert(colCount<std::tuple<int, std::array<char, 4>>>::value == 5, "");
  static_assert(colCount<std::tuple<>>::value == 0, "");
}
}
}
