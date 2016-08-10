/*!
 * @file
 * Basic tests for `ReduceAll.hpp`
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

#include <ezl/units/ReduceAll.hpp>
#include <ezl/helper/meta/typeInfo.hpp>
#include <ctorTeller.hpp>

namespace ezl {
namespace test {
using namespace ezl::detail;

void ReduceAllBasicCallTest();
void ReduceAllValPerformanceTest();
void ReduceAllKeyPerformanceTest();
void ReduceAllReturnPerformanceTest();
void ReduceAllOrderedTest();
void ReduceAllBunchTest();
void ReduceAllAdjacentTest();

void ReduceAllTest(int argc, char* argv[]) {
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator comm;
  ReduceAllBasicCallTest();
  ReduceAllOrderedTest();
  ReduceAllBunchTest();
  ReduceAllAdjacentTest();
  /*
  ReduceAllValPerformanceTest();
  ReduceAllKeyPerformanceTest();
  ReduceAllReturnPerformanceTest();
  */
}

void ReduceAllOrderedTest() {
  using std::make_tuple;
  using std::vector;
  using meta::slct;
  using std::tuple;
  using ezl::detail::meta::ReduceAllTypes;

  auto t1 = make_tuple(3, 'c', 4.);
  auto f1 = [](char c, vector<int> v) { 
    return v.size(); 
  };

  auto count = 0;
  auto f2 = [&count](const size_t& precount, const vector<tuple<>>& i) {
    count = precount;
    return i;
  };

  auto r1 = std::make_shared<ReduceAll<ReduceAllTypes<decltype(t1), slct<2>, slct<1>, decltype(f1),
       slct<2>>>>(f1, true, false, 0, false);

  auto ret = std::make_shared<ReduceAll<ReduceAllTypes<std::tuple<const size_t&>, slct<1>, slct<>, decltype(f2),
    slct<1>>>>(f2, true, false, 0, false);
  r1->next(ret, r1);

  r1->dataEvent(t1);
  r1->dataEvent(t1);
  assert(count == 0);
  r1->dataEvent(make_tuple(2,'g', 5.));
  r1->dataEvent(make_tuple(2,'h', 5.));
  assert(count == 2);
}

void ReduceAllAdjacentTest() {
  using std::make_tuple;
  using std::vector;
  using meta::slct;
  using std::tuple;
  using ezl::detail::meta::ReduceAllTypes;

  auto t1 = make_tuple(3, 'c', 4.);
  auto t2 = make_tuple(3, 'd', 4.);

  std::vector<size_t> count;
  auto f1 = [&count](char c, vector<int> v) { 
    count.push_back(v.size());
    return v.size(); 
  };

  auto r1 = std::make_shared<ReduceAll<ReduceAllTypes<decltype(t1), slct<2>, slct<1>, decltype(f1),
       slct<2>>>>(f1, true, true, 3, true);

  r1->dataEvent(t1);
  r1->dataEvent(t2);
  r1->dataEvent(t2);
  r1->dataEvent(t2);
  r1->dataEvent(t1);
  r1->dataEvent(t1);
  r1->dataEvent(t1);
  r1->dataEvent(t1);
  r1->signalEvent(1);

  assert(count.size() == 5);
  assert(count[0] == 1);
  assert(count[1] == 3);
  assert(count[2] == 2);
  assert(count[3] == 3);
  assert(count[4] == 3);
}

void ReduceAllBunchTest() {
  using std::make_tuple;
  using std::vector;
  using meta::slct;
  using std::tuple;
  using ezl::detail::meta::ReduceAllTypes;

  auto t1 = make_tuple(3, 'c', 4.);
  auto t2 = make_tuple(3, 'd', 4.);

  std::vector<size_t> count;
  auto f1 = [&count](char c, vector<int> v) { 
    count.push_back(v.size());
    return v.size(); 
  };

  auto r1 = std::make_shared<ReduceAll<ReduceAllTypes<decltype(t1), slct<2>, slct<1>, decltype(f1),
       slct<2>>>>(f1, true, false, 3, false);

  r1->dataEvent(t1);
  r1->dataEvent(t2);
  r1->dataEvent(t2);
  r1->dataEvent(t2);
  r1->dataEvent(t1);
  r1->dataEvent(t1);
  r1->dataEvent(t1);
  r1->dataEvent(t1);
  r1->signalEvent(1);

  assert(count.size() == 4);
  assert(count[0] == 1);
  assert(count[1] == 3);
  assert(count[2] == 3);
  assert(count[3] == 1);
}

void ReduceAllKeyPerformanceTest() {
}

void ReduceAllReturnPerformanceTest() {
}

void ReduceAllBasicCallTest() {
  using std::make_tuple;
  using meta::slct;
  using std::tuple;
  using ezl::detail::meta::ReduceAllTypes;

  auto t1 = make_tuple(3, 'c', 4.);
  char ch;

  auto f1 = [&ch](char c, std::vector<int> ) { 
    ch = c; 
    return 1; 
  };
  ReduceAll<ReduceAllTypes<decltype(t1), slct<2>, slct<1>, decltype(f1), slct<1,2>>> r1{f1, false, false, 0, false};
  r1.dataEvent(t1);
  r1.signalEvent(1);
  assert(ch == 'c');

  auto f2 = [&ch](char c, std::vector<std::tuple<>>) { 
    ch = c; 
    return 1; 
  };
  ReduceAll<ReduceAllTypes<decltype(t1), slct<2>, slct<>, decltype(f2), slct<1,2>>> r2{f2, false, false, 0, false};
  r2.dataEvent(t1);

  auto f3 = [&ch](std::vector<char> c) { 
    return 1; 
  };
  ReduceAll<ReduceAllTypes<decltype(t1), slct<>, slct<2>, decltype(f3), slct<1>>> r3{f3, false, false, 0, false};
  r3.dataEvent(t1);

  auto f4 = [&ch](char c, std::vector<int>, std::vector<double> ) { 
    ch = c; 
    return 1; 
  };
  ReduceAll<ReduceAllTypes<decltype(t1), slct<2>, slct<1,3>, decltype(f4), slct<1,2>>> r4{f4, false, false, 0, false};
  r4.dataEvent(t1);

  auto f5 = [&ch](std::tuple<char> c, std::tuple<std::vector<int>, std::vector<double>> ) { 
    return 1; 
  };
  ReduceAll<ReduceAllTypes<decltype(t1), slct<2>, slct<1,3>, decltype(f5), slct<1,2>>> r5{f5, false, false, 0, false};
  r5.dataEvent(t1);


  auto f6 = [&ch](char c, std::vector<std::tuple<int, double>> ) { 
    ch = c; 
    return 1; 
  };
  ReduceAll<ReduceAllTypes<decltype(t1), slct<2>, slct<1,3>, decltype(f6), slct<1,2>>> r6{f6, false, false, 0, false};
  r6.dataEvent(t1);
}

void ReduceAllValPerformanceTest() {}
}
}
