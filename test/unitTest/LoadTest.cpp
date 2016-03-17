/*!
 * @file
 * Basic tests for `Load.hpp`
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

#include <ezl/mapreduce/Load.hpp>
#include <ezl/mapreduce/Filter.hpp>
#include <ctorTeller.hpp>

using namespace ezl::detail;

void LoadBasicTest();

void LoadTest(int argc, char* argv[]) {
  LoadBasicTest();
}

void LoadBasicTest() {
  using std::make_tuple;
  using std::make_shared;
  using std::vector;
  using std::array;
  using meta::slct;
  using std::tuple;
  using std::make_pair;

  auto count = 0;
  auto isMore = true;
  auto f1 = [&isMore]() { 
    auto f = isMore; 
    isMore = !isMore;  
    return make_pair(2, f);
  };
  auto r1 = make_shared<Load<decltype(f1)>>(ProcReq{1}, std::move(f1), nullptr);
  auto fret = [&count]() { ++count; return 0; };
  auto ret = make_shared<Filter<decltype(r1)::element_type::otype, slct<>, decltype(fret),slct<1>>>(fret);
  auto pr = Par{vector<int>{0}, array<int, 3>{{1,2,3}}, 0};
  r1->par(pr);
  r1->next(ret, r1); 
  r1->pull();
  assert(count == 1);

  count = 0;
  auto f2 = []() { return make_pair(make_tuple(2), false); };
  auto r2 = make_shared<Load<decltype(f2)>>(ProcReq{1}, std::forward<decltype(f2)>(f2), nullptr);
  r2->par(pr);
  r2->next(ret, r2); 
  r2->pull();
  assert(count == 0);

  count = 0;
  vector<int> v{2, 3};
  auto f3 = [&v]() { return std::move(v); };
  auto r3 = make_shared<Load<decltype(f3)>>(ProcReq{1}, std::move(f3), nullptr);
  r3->par(pr);
  r3->next(ret, r3); 
  r3->pull();
  assert(count == 2);

  count = 0;
  auto f4 = []() { return vector<tuple<int>>{}; };
  auto r4 = make_shared<Load<decltype(f4)>>(ProcReq{1}, std::move(f4), nullptr);
  r4->par(pr);
  r4->next(ret, r4); 
  r4->pull();
  assert(count == 0);

}
