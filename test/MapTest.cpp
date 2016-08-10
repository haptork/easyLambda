/*!
 * @file
 * Basic tests for `Map.hpp`
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

#include <ezl/units/Map.hpp>
#include <ezl/helper/meta/typeInfo.hpp>

#include <ctorTeller.hpp>

namespace ezl {
namespace test {

using namespace ezl::detail;

auto f2() {  // used as UDF in a test
  return 0;
}

void MapBasicCallTest();
void MapCopyPerformanceTest();
void MapReturnPerformanceTest();

void MapTest(int argc, char* argv[]) {
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator comm;
  MapBasicCallTest();
  MapCopyPerformanceTest();
  MapReturnPerformanceTest();
}
  
void MapReturnPerformanceTest() {
  using std::tuple;
  using std::make_tuple;
  using meta::slct;
  using std::vector;
  using ezl::detail::meta::MapTypes;

  auto f = [](const std::tuple<const ctorTeller&>& ) { return 3; };
  auto rec = std::make_shared<Map<MapTypes<tuple<const ctorTeller&>, slct<1>, decltype(f), slct<2>>>>(f);

  auto vecTupRet = [](const int& ) { 
    vector<tuple<ctorTeller>> v;
    v.push_back(std::move(std::tuple<ctorTeller>{ctorTeller("Map-Return", true, false)}));
    assert(ctorTeller::movector() == 2);
    assert(ctorTeller::copyctor() == 0);
    return v;
  };
  auto vecTupRetMap = std::make_shared<Map<MapTypes<tuple<int>, slct<1>, decltype(vecTupRet), slct<2>>>>(vecTupRet);
  vecTupRetMap->next(rec, vecTupRetMap);

  auto tupRet = [](const int& ) { 
    std::tuple<ctorTeller> t {ctorTeller("Map-Return", true, false)};
    assert(ctorTeller::movector() == 1);
    assert(ctorTeller::copyctor() == 0);
    return t;
  };
  auto tupRetMap = std::make_shared<Map<MapTypes<tuple<int>, slct<1>, decltype(tupRet), slct<2>>>>(tupRet);
  tupRetMap->next(rec, tupRetMap);

  auto plainRet = [](const int& ) { 
    return ctorTeller{"Map-Return", true, false};
  };
  auto plainRetMap =
      std::make_shared<Map<MapTypes<tuple<int>, slct<1>, decltype(plainRet), slct<2>>>>(plainRet);
  //plainRetMap->next(rec, plainRetMap);


  auto vecPlainRet = [](const int& ) { 
    vector<ctorTeller> v;
    v.emplace_back("Map-Return", true, false);
    assert(ctorTeller::movector() == 0);
    assert(ctorTeller::copyctor() == 0);
    return v;
  };
  auto vecPlainRetMap = std::make_shared<Map<MapTypes<tuple<int>, slct<1>, decltype(vecPlainRet), slct<2>>>>(vecPlainRet);
  vecPlainRetMap->next(rec, vecPlainRetMap);

  vecTupRetMap->dataEvent(std::make_tuple(1));
  //ctorTeller::display();
  assert(ctorTeller::movector() == 2);  // same as UDF
  assert(ctorTeller::copyctor() == 0);


  tupRetMap->dataEvent(std::make_tuple(1));
  //ctorTeller::display();
  assert(ctorTeller::movector() == 1);  // same as UDF
  assert(ctorTeller::copyctor() == 0);

  plainRetMap->dataEvent(std::make_tuple(1));
  //ctorTeller::display();
  assert(ctorTeller::movector() == 0);
  assert(ctorTeller::copyctor() == 0);

  vecPlainRetMap->dataEvent(std::make_tuple(1));
  //ctorTeller::display();
  assert(ctorTeller::movector() == 0);
  assert(ctorTeller::copyctor() == 0);
/*
  auto vecRet3 = [](const int& ) { 
    vector<tuple<ctorTeller>> v;
    v.emplace_back(ctorTeller{"Map-Return", true, true});
    assert(ctorTeller::movector() == 1);
    assert(ctorTeller::copyctor() == 0);
    assert(ctorTeller::ctor() == 1);
    return v;
  };

  auto vecRet2 = [](const int& ) { 
    vector<tuple<ctorTeller>> v;
    v.push_back(std::tuple<ctorTeller>{ctorTeller("Map-Return", true, true)});
    ctorTeller::display();    
    return v;
  };
*/
}

void MapBasicCallTest() {
  using std::make_tuple;
  using meta::slct;
  using std::tuple;
  using ezl::detail::meta::MapTypes;

  auto t1 = make_tuple(3, 'c', 4.);
  char r1;
  auto f1 = [&r1](char ch) { 
    r1 = ch; 
    return false; 
  };
  Map<MapTypes<decltype(t1), slct<2>, decltype(f1), slct<1,4>>> m1{f1};
  m1.dataEvent(t1);
  assert(r1 == 'c');

  Map<MapTypes<std::tuple<>, slct<>, decltype(&f2), slct<1>>> m2{f2};
  m2.dataEvent(std::tuple<>{});
}

void MapCopyPerformanceTest() {
  using std::make_tuple;
  using meta::slct;
  using std::tuple;
  using ezl::detail::meta::MapTypes;

  auto t2 = make_tuple(ctorTeller("Map: 1", true, false), 1, 'c');
  assert(ctorTeller::movector() == 1);
  assert(ctorTeller::copyctor() == 0);
  auto g1 = [](ctorTeller, int ) { return 0; };
  auto g2 = [](std::tuple<ctorTeller, int> ) { return 0; };
  auto g3 = [](const std::tuple<ctorTeller, int>& ) { return 0; };
  auto g4 = [](const std::tuple<const ctorTeller&, const int&>& ) { return 0; };
  auto g5 = [](const ctorTeller&, const int& ) { return 0; };

  auto n1 = std::make_shared<Map<MapTypes<decltype(t2), slct<1, 2>, decltype(g1), slct<3, 1>>>>(g1);
  auto n2 = std::make_shared<Map<MapTypes<decltype(t2), slct<1, 2>, decltype(g2), slct<3, 1>>>>(g2);
  auto n3 = std::make_shared<Map<MapTypes<decltype(t2), slct<1, 2>, decltype(g3), slct<3, 1>>>>(g3);
  auto n4 = std::make_shared<Map<MapTypes<decltype(t2), slct<1, 2>, decltype(g4), slct<3, 1>>>>(g4);
  auto n5 = std::make_shared<Map<MapTypes<decltype(t2), slct<1, 2>, decltype(g5), slct<3, 1>>>>(g5);

  n1->dataEvent(t2); // one copy 
  n2->dataEvent(t2); // one copy    
  n3->dataEvent(t2); // one copy  
  assert(ctorTeller::copyctor() == 3);
  assert(ctorTeller::movector() == 1);

  auto t3 = make_tuple(ctorTeller("Map: 2", true, false), 3);

  assert(ctorTeller::copyctor() == 0);
  assert(ctorTeller::movector() == 1);
  n4->dataEvent(t2); // no copy  
  n5->dataEvent(t2); // no copy
  assert(ctorTeller::copyctor() == 0);
  assert(ctorTeller::movector() == 1);

  auto h1 = [](const std::tuple<const ctorTeller&, const int&>& ) { return 3; };
  auto h3 = [](const int& ) { return 4; };

  auto x1 = std::make_shared<Map<MapTypes<decltype(t3), slct<1, 2>, decltype(h1), slct<3>>>>(h1);
  auto y1 = std::make_shared<Map<MapTypes<tuple<const int&>, slct<1>, decltype(h3), slct<2>>>>(h3);


  x1->next(y1, x1);
  x1->dataEvent(t3); // no copy
  assert(ctorTeller::copyctor() == 0);
  assert(ctorTeller::movector() == 1);

  using tt = tuple<const ctorTeller&, const int&>;
  auto h2 = [](const ctorTeller&, const int& ) { return 4; };

  auto x2 = std::make_shared<Map<MapTypes<decltype(t3), slct<1, 2>, decltype(h2), slct<1,2>>>>(h2);
  auto y2 = std::make_shared<Map<MapTypes<tt, slct<1,2>, decltype(h2), slct<1,2>>>>(h2);
  x2->next(y2, x2);
  x2->dataEvent(t3); // no copy
  assert(ctorTeller::copyctor() == 0);
  assert(ctorTeller::movector() == 1);
}
}
}
