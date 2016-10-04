/*! 
 * @file
 * demo examples for predicates without any other usage of ezl library.
 *
 * Taken from the blog post: https://haptork.github.io/easyLambda/algorithm/easy-unary-predicates/ 
 * */

#include <algorithm>
#include <array>
#include <assert.h>
#include <iostream>
#include <map>
#include <vector>

#include <ezl/algorithms/predicates.hpp>

using std::vector;
using std::tuple;
using std::count_if;
using std::pair;
using std::make_pair;
using std::make_tuple;
using std::array;

using ezl::gt;
using ezl::lt;
using ezl::eq;
using ezl::tautology;

auto example1() {
  auto a = gt(80) || lt(40);
  auto a_notone_odd = a && !eq(1) && ([](int x) { return x % 2; });

  assert(a(3) == true);
  assert(a_notone_odd(1) == false);
}

auto example2() {
  // adding junk values to a vector
  auto v = vector<tuple<int, char, int>>{};
  v.emplace_back(1, 'a', 11);
  v.emplace_back(2, 'b', 22);
  v.emplace_back(3, 'c', 33);
  
  auto n = count_if(begin(v), end(v), eq(3, 'c', 33) || lt<2>('b'));
  assert(n == 2);
}

auto example3() {
  // If a point is inside a circle of unit radius & center at origin
  auto in_unit_circle = [] (auto p) { 
    auto x = std::get<0>(p), y = std::get<1>(p);
    return (x*x + y*y) < 1;
  };
  auto points = vector<array<double, 2>> {{.1, .2}, {-5., .1}, 
                                          {5., .1}, {-0.4, -.9}};
  // In first quadrant and in unit circle 
  auto a =  gt<1>(0.) && gt<2>(0.) && in_unit_circle;
  // In second, third or fourth quadrant or outside unit circle
  auto b = lt<1>(0.) || lt<2>(0.) || !(tautology() && in_unit_circle);
  // a == !b;
  for_each(begin(points), end(points), [&](auto x) { assert(a(x) == !b(x)); });

  assert(a(tuple<double, double>{0.3, 0.8}) == true);
  assert(a(array<double, 2>{0.45, 0.9}) == false);
  assert(a(pair<double, double>{0.3, -0.3}) == false);

  auto n = count_if(begin(points), end(points), a);
  assert(n == 1);
}

int main(int argc, char* argv[]) {
  example1();
  example2();
  example3();
  return 0;
}
