/*!
 * @file
 * Basic tests for `fromFile.hpp`
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
#include <vector>
#include <type_traits>
#include <assert.h>

#include <boost/mpi.hpp>

#include <ezl/algorithms/fromFile.hpp>
#include <ezl/helper/ProcReq.hpp>
#include <ezl/units/Filter.hpp>
#include <ezl/units/Rise.hpp>

namespace ezl {
namespace test {
using namespace ezl::detail;

void fromFileBasicTest();

void fromFileTest(int argc, char* argv[]) {
  fromFileBasicTest();
}

void fromFileStrictSchemaTest();
void fromFileFileNameTest();
void fromFileRowMaxTest();

void fromFileBasicTest() {
  fromFileStrictSchemaTest();
  fromFileFileNameTest();
  fromFileRowMaxTest();
  //fromFileDelimTest();
  //fromFileSlctTest();
  //fromFilePreCheckTest();
}

void fromFileFileNameTest() {
  using meta::slct;
  using std::tuple;
  using std::string;
  using std::vector;
  using std::array;
  using ezl::ProcReq;
  using ezl::Par;

  // r1 declaration and setting properties be in different statements different
  auto r1 = ezl::fromFile<string, array<float, 2>, string>(
          "data/fromFileTests/test?.txt");
  r1 = r1.addFileName();
  auto t1 = std::make_shared<Rise<decltype(r1)>>(ProcReq{}, std::move(r1), nullptr);
  auto count = 0;
  auto f1 = [&count](string f){
    if(f.size()>=9 && f.substr(f.size()-9, 9) == "test1.txt")
      count++;
    return true;
  };
  auto ret = std::make_shared<Filter<decltype(t1)::element_type::otype, slct<3>, decltype(f1), slct<>>>(f1);
  t1->next(ret, t1);
  t1->par(Par{vector<int>{0}, array<int, 3>{{1,2,3}}, 0});
  t1->pull();
  assert(count == 3);
}

void fromFileRowMaxTest() {
  using meta::slct;
  using std::tuple;
  using std::string;
  using std::vector;
  using std::array;
  using ezl::ProcReq;
  using ezl::Par;

  auto r1 = ezl::fromFile<string, int, float, string>("data/fromFileTests/test?.txt");
  r1 = r1.addFileName().limitRows(2);
  auto t1 =
      std::make_shared<Rise<decltype(r1)>>(ProcReq{}, std::move(r1), nullptr);
  auto count = 0;
  auto f1 = [&count](){ count++; return true; };
  auto ret = std::make_shared<Filter<decltype(t1)::element_type::otype, slct<>, decltype(f1), slct<>>>(f1);
  t1->next(ret, t1);
  t1->par(Par{vector<int>{0}, array<int, 3>{{1,2,3}}, 0});
  t1->pull();
  assert(count == 2);
}

void fromFileStrictSchemaTest() {
  using meta::slct;
  using std::tuple;
  using std::string;
  using std::vector;
  using std::array;
  using ezl::ProcReq;
  using ezl::Par;

  auto r1 = ezl::fromFile<string, int, float>("data/fromFileTests/test?.txt");
  auto t1 =
      std::make_shared<Rise<decltype(r1)>>(ProcReq{}, std::move(r1), nullptr);

  auto count = 0;
  auto f1 = [&count](){ count++; return true; };
  auto ret = std::make_shared<Filter<decltype(t1)::element_type::otype, slct<>, decltype(f1), slct<>>>(f1);
  t1->next(ret, t1);
  t1->par(Par{vector<int>{0}, array<int, 3>{{1,2,3}}, 0});
  t1->pull();
  assert(count == 6);

  auto r2 = ezl::fromFile<string, int, float>("data/fromFileTests/test?.txt");
  r2 = r2.strictSchema(false);
  auto t2 =
      std::make_shared<Rise<decltype(r2)>>(ProcReq{}, std::move(r2), nullptr);

  count = 0;
  t2->next(ret, t2);
  t2->par(Par{vector<int>{0}, array<int, 3>{{1,2,3}}, 0});
  t2->pull();
  assert(count == 14);

  auto r3 = ezl::fromFile<string, int>("data/fromFileTests/test?.txt");
  auto t3 =
      std::make_shared<Rise<decltype(r3)>>(ProcReq{}, std::move(r3), nullptr);

  count = 0;
  auto ret2 = std::make_shared<Filter<decltype(t3)::element_type::otype, slct<>, decltype(f1), slct<>>>(f1);
  t3->next(ret2, t3);
  t3->par(Par{vector<int>{0}, array<int, 3>{{1,2,3}}, 0});
  t3->pull();
  assert(count == 6);

  auto r4 = ezl::fromFile<string, int>("data/fromFileTests/test?.txt");
  r4 = r4.strictSchema(false);
  auto t4 =
      std::make_shared<Rise<decltype(r4)>>(ProcReq{}, std::move(r4), nullptr);

  count = 0;
  t4->next(ret2, t4);
  t4->par(Par{vector<int>{0}, array<int, 3>{{1,2,3}}, 0});
  t4->pull();
  assert(count == 14);
}
}
}
