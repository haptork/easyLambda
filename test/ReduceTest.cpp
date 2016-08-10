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

#include <boost/mpi.hpp>

#include <ezl/units/Reduce.hpp>
#include <ezl/helper/meta/typeInfo.hpp>
#include <ctorTeller.hpp>

namespace ezl {
namespace test {
using namespace ezl::detail;

void ReduceBasicCallTest();
void ReduceValPerformanceTest();
void ReduceKeyPerformanceTest();
void ReduceReturnPerformanceTest();
void ReduceOrderedTest();
void ReduceReturnRefPerformanceTest();

void ReduceTest(int argc, char* argv[]) {
  using ezl::detail::meta::ReduceTypes;  

  ReduceBasicCallTest();
  ReduceValPerformanceTest();
  ReduceKeyPerformanceTest();

  ReduceReturnPerformanceTest();

  ReduceReturnRefPerformanceTest();
  ReduceOrderedTest();

}

void ReduceReturnRefPerformanceTest() {
  using std::tuple;
  using std::make_tuple;
  using meta::slct;
  using std::vector;
  using ezl::detail::meta::ReduceTypes;

  auto init = ctorTeller("Reduce: init", false, false);
  auto f = [](const int&, const int&, const int& ) { return 3; };

  auto rec = std::make_shared<
      Reduce<ReduceTypes<tuple<const ctorTeller &, const int &>, slct<2>, slct<2>,
         decltype(f), int, slct<1, 2>>>>(f, 0, false, false);

  auto* two = &init;

  auto refRet = [&two](ctorTeller& r, int, int a) -> ctorTeller& {
    r.val = a;
    two = &r;
    return r;
  };
  auto refRetReduce = std::make_shared<
      Reduce<ReduceTypes<tuple<int>, slct<1>, slct<1>, decltype(refRet), decltype(init),
             slct<2, 1>>>>(refRet, init, false, false);
  refRetReduce->next(rec, refRetReduce);

  ctorTeller::reset();
  refRetReduce->dataEvent(std::make_tuple(2));
  assert(two->val == 2);
  refRetReduce->dataEvent(std::make_tuple(1));
  refRetReduce->dataEvent(std::make_tuple(1));
  refRetReduce->dataEvent(std::make_tuple(1));
  assert(two->val == 1);
  refRetReduce->signalEvent(1);
  assert(ctorTeller::movector() == 0);
  assert(ctorTeller::copyctor() == 2);
  assert(ctorTeller::moveassign() == 0);
  assert(ctorTeller::copyassign() == 0);

}

void ReduceOrderedTest() {
  using std::make_tuple;
  using meta::slct;
  using std::tuple;
  using ezl::detail::meta::ReduceTypes;
  auto t1 = make_tuple(3, 'c', 4.);
  auto f1 = [](int i, char c) { 
    return i+1; 
  };

  auto count = 0;
  auto f2 = [&count](int i, int precount) {
    count = precount;
    return i;
  };

  auto r1 = std::make_shared<Reduce<ReduceTypes<decltype(t1), slct<2>, slct<>, decltype(f1), int, 
    slct<2>>>>(f1, 0, false, true);

  auto ret = std::make_shared<Reduce<ReduceTypes<std::tuple<const int&>, slct<>, slct<1>, decltype(f2), int, 
    slct<1>>>>(f2, 0, false, false);
  r1->next(ret, r1);

  r1->dataEvent(t1);
  r1->dataEvent(t1);
  assert(count == 0);
  r1->dataEvent(make_tuple(2,'g', 5.));
  assert(count == 2);
}

void ReduceKeyPerformanceTest() {
  using std::make_tuple;
  using meta::slct;
  using std::tuple;
  using std::tie;
  using ezl::detail::meta::ReduceTypes;

  auto t1 = make_tuple(ctorTeller("Reduce: 1", true, false), 1, 3);
  auto t2 = tie(std::get<0>(t1), std::get<1>(t1), std::get<2>(t1));
  auto g1 = [](int, ctorTeller, int) { return 0; };
  auto g2 = [](int, std::tuple<ctorTeller>, std::tuple<int>) { return 0; };
  auto g3 = [](int, const std::tuple<ctorTeller>&, const std::tuple<int>&) { return 0; };
  auto g4 = [](const int &, 
               const std::tuple<const ctorTeller &> &,
               const std::tuple<const int &>) { return 0; };
  auto g5 = [](const int&, const ctorTeller&, const int&) { return 0; };

  auto n1 = std::make_shared<Reduce<ReduceTypes<decltype(t2), slct<1>, slct<2>, decltype(g1), int, 
    slct<1,2>>>>(g1, 0, false, false);
  auto n2 = std::make_shared<Reduce<ReduceTypes<decltype(t2), slct<1>, slct<2>, decltype(g2), int, 
    slct<1,2>>>>(g2, 0, false, false);
  auto n3 = std::make_shared<Reduce<ReduceTypes<decltype(t2), slct<1>, slct<2>, decltype(g3), int, 
    slct<1,2>>>>(g3, 0, false, false);
  auto n4 = std::make_shared<Reduce<ReduceTypes<decltype(t2), slct<1>, slct<2>, decltype(g4), int, 
    slct<1,2>>>>(g4, 0, false, false);
  auto n5 = std::make_shared<Reduce<ReduceTypes<decltype(t2), slct<1>, slct<2>, decltype(g5), int, 
    slct<1,2>>>>(g5, 0, false, false);

  ctorTeller::reset();

// calling n5 with t1 / u1 causes compile issues with gcc
#ifdef __clang__  // TODO also for gcc
  n5->dataEvent(t2);
  assert(ctorTeller::movector() == 1);
  assert(ctorTeller::copyctor() == 1);
  n5->dataEvent(t1);
  n5->dataEvent(t2);
  assert(ctorTeller::movector() == 1);
  assert(ctorTeller::copyctor() == 1);

  auto u1 = make_tuple(ctorTeller("2", false, false), 1, 3);
  ctorTeller::reset();
  n5->dataEvent(u1);
  n5->dataEvent(t2);
  n5->dataEvent(u1);
  assert(ctorTeller::movector() == 1);
  assert(ctorTeller::copyctor() == 1);
#endif
}

void ReduceReturnPerformanceTest() {
  using std::tuple;
  using std::make_tuple;
  using meta::slct;
  using std::vector;
  using ezl::detail::meta::ReduceTypes;

  auto init = ctorTeller("Reduce: init", false, false);
  auto tinit = make_tuple(init);
  auto vinit = std::vector<ctorTeller> {init};
  auto vtinit = std::vector<tuple<ctorTeller>> {tinit};
  auto f = [](const int&, const int&, const int& ) { return 3; };
  auto rec = std::make_shared<
      Reduce<ReduceTypes<tuple<const ctorTeller &, const int &>, slct<2>, slct<2>,
         decltype(f), int, slct<1, 2>>>>(f, 0, false, false);
  auto vecTupRet = [](const vector<tuple<ctorTeller>>&, const int&, const int&) { 
    vector<tuple<ctorTeller>> v;
    v.push_back(std::move(
        std::tuple<ctorTeller>{ctorTeller("Reduce-Return", false, false)}));
    return v;
  };
  auto vecTupRetReduce = std::make_shared<
      Reduce<ReduceTypes<tuple<int>, slct<1>, slct<1>, decltype(vecTupRet), decltype(vtinit),
             slct<2, 1>>>>(vecTupRet, vtinit, false, false);
  vecTupRetReduce->next(rec, vecTupRetReduce);

  auto tupRet = [](const std::tuple<const ctorTeller&>&, const int&, const int&) { 
    return std::make_tuple(ctorTeller{"Reduce-Return", false, false});
  };
  auto tupRetReduce =
      std::make_shared<Reduce<ReduceTypes<tuple<int>, slct<1>, slct<1>, decltype(tupRet), 
      decltype(tinit), slct<2,1>>>>(tupRet, tinit, false, false);
  tupRetReduce->next(rec, tupRetReduce);

  auto plainRet = [](const ctorTeller&, const int&, const int&) { 
    return ctorTeller{"Reduce-Return", false, false};
  };
  auto plainRetReduce = std::make_shared<
      Reduce<ReduceTypes<tuple<int>, slct<1>, slct<1>, decltype(plainRet), decltype(init),
             slct<2, 1>>>>(plainRet, init, false, false);
  plainRetReduce->next(rec, plainRetReduce);

  auto vecPlainRet = [](const vector<ctorTeller>&, const int&, const int&) { 
    vector<ctorTeller> v;
    v.emplace_back("Reduce-Return", false, false);
    return v;
  };
  auto vecPlainRetReduce = std::make_shared<Reduce<ReduceTypes<tuple<int>, slct<1>, slct<1>,
       decltype(vecPlainRet), decltype(vinit), slct<2,1>>>>(vecPlainRet, vinit, false, false);
  vecPlainRetReduce->next(rec, vecPlainRetReduce);

  ctorTeller::reset();
  vecTupRetReduce->dataEvent(std::make_tuple(1));
  vecTupRetReduce->dataEvent(std::make_tuple(2));
  vecTupRetReduce->dataEvent(std::make_tuple(1));
  vecTupRetReduce->dataEvent(std::make_tuple(3));
  vecTupRetReduce->dataEvent(std::make_tuple(1));
  vecTupRetReduce->signalEvent(1);
  assert(ctorTeller::movector() == 10);  // same as UDF
  assert(ctorTeller::copyctor() == 0);
  assert(ctorTeller::copyassign() == 0);
  assert(ctorTeller::moveassign() == 0);

  ctorTeller::reset();

  tupRetReduce->dataEvent(std::make_tuple(1));
  tupRetReduce->dataEvent(std::make_tuple(2)); //TODO: why a new key is adding move
  tupRetReduce->dataEvent(std::make_tuple(3));
  tupRetReduce->dataEvent(std::make_tuple(1));
  tupRetReduce->dataEvent(std::make_tuple(1));
  tupRetReduce->signalEvent(1);
  assert(ctorTeller::movector() == 8);  // more than UDF
  assert(ctorTeller::copyctor() == 0);
  assert(ctorTeller::moveassign() == 2); // TODO: why no RVO?
  assert(ctorTeller::copyassign() == 0);

  ctorTeller::reset();
  plainRetReduce->dataEvent(std::make_tuple(1));
  plainRetReduce->dataEvent(std::make_tuple(2));
  plainRetReduce->dataEvent(std::make_tuple(3));
  plainRetReduce->dataEvent(std::make_tuple(1));
  plainRetReduce->dataEvent(std::make_tuple(1));
  plainRetReduce->signalEvent(1);
  assert(ctorTeller::movector() == 3);
  assert(ctorTeller::copyctor() == 0);
  assert(ctorTeller::moveassign() == 2);
  assert(ctorTeller::copyassign() == 0);

  ctorTeller::reset();
  vecPlainRetReduce->dataEvent(std::make_tuple(1));
  vecPlainRetReduce->dataEvent(std::make_tuple(1));
  vecPlainRetReduce->dataEvent(std::make_tuple(3));
  vecPlainRetReduce->dataEvent(std::make_tuple(1));
  vecPlainRetReduce->signalEvent(1);
  assert(ctorTeller::movector() == 0);
  assert(ctorTeller::copyctor() == 0);
  assert(ctorTeller::moveassign() == 0);
  assert(ctorTeller::copyassign() == 0);
}

void ReduceBasicCallTest() {
  using std::make_tuple;
  using meta::slct;
  using std::tuple;
  using ezl::detail::meta::ReduceTypes;

  auto t1 = make_tuple(3, 'c', 4.);
  char ch;
  auto f1 = [&ch](int i, char c) { 
    ch = c; 
    return i+1; 
  };
  Reduce<ReduceTypes<decltype(t1), slct<2>, slct<>, decltype(f1), int, slct<1,2>>> r1{f1, 0, false, false};
  r1.dataEvent(t1);
  assert(ch == 'c');
}


void ReduceValPerformanceTest() {
  using std::make_tuple;
  using meta::slct;
  using std::tuple;
  using ezl::detail::meta::ReduceTypes;

  auto t2 = make_tuple(ctorTeller("Reduce: 1", true, false), 1, 3);
  assert(ctorTeller::movector() == 1);
  assert(ctorTeller::copyctor() == 0);
  auto g1 = [](int, int, ctorTeller) { return 0; };

  auto g2 = [](int, std::tuple<int>, std::tuple<ctorTeller>) { return 0; };
  auto g3 = [](int, const std::tuple<int> &, const std::tuple<ctorTeller> &) {
    return 0;
  };
  auto g4 = [](const int &, const std::tuple<const int &>,
               const std::tuple<const ctorTeller &> &) { return 0; };
  auto g5 = [](const int &, const int &, const ctorTeller &) { return 0; };

  auto n1 =
      std::make_shared<Reduce<ReduceTypes<decltype(t2), slct<2>, slct<1>, decltype(g1), int,
                              slct<1, 2>>>>(g1, 0, false, false);
  auto n2 =
      std::make_shared<Reduce<ReduceTypes<decltype(t2), slct<2>, slct<1>, decltype(g2), int,
                              slct<1, 2>>>>(g2, 0, false, false);
  auto n3 =
      std::make_shared<Reduce<ReduceTypes<decltype(t2), slct<2>, slct<1>, decltype(g3), int,
                              slct<1, 2>>>>(g3, 0, false, false);
  auto n4 =
      std::make_shared<Reduce<ReduceTypes<decltype(t2), slct<2>, slct<1>, decltype(g4), int,
                              slct<1, 2>>>>(g4, 0, false, false);
  auto n5 =
      std::make_shared<Reduce<ReduceTypes<decltype(t2), slct<2>, slct<1>, decltype(g5), int,
                              slct<1, 2>>>>(g5, 0, false, false);

  assert(ctorTeller::copyctor() == 0);
  n1->dataEvent(t2); // one copy
  assert(ctorTeller::copyctor() == 1);
  n2->dataEvent(t2); // one copy    
  n3->dataEvent(t2); // one copy  
  assert(ctorTeller::copyctor() == 3);
  assert(ctorTeller::movector() == 1);
  n4->dataEvent(t2); // no copy  
  n5->dataEvent(t2); // no copy
  assert(ctorTeller::copyctor() == 3);
  assert(ctorTeller::movector() == 1);
}
}
}