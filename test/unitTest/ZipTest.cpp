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
#include <boost/functional/hash.hpp>

#include <ezl/mapreduce/Filter.hpp>
#include <ezl/mapreduce/Zip.hpp>

using namespace ezl::detail;

void ZipBasicCallTest();

void ZipTest(int argc, char* argv[]) {
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator comm;
  ZipBasicCallTest();
}

void ZipBasicCallTest() {
  using std::make_tuple;
  using meta::slct;
  using std::tuple;


  auto t1 = make_tuple(3, 'c', 4.);
  auto t2 = make_tuple(4, 'c');
  char ch;
  auto f1 = [&ch](char c, int i) { 
    ch = c; 
    return i+1; 
  };
  auto r1 = std::make_shared<
  Filter<tuple<const int&, const char&>, slct<2,1>, decltype(f1), slct<1,2>>>(f1);

  auto z1 = std::make_shared<
  Zip<decltype(t1), decltype(t2), slct<1>, slct<1>, slct<1,2>, boost::hash<std::tuple<int>>>>();
  z1->next(r1, z1);
  z1->dataEvent(t1);
  z1->dataEvent(t2);
  std::get<0>(t2) = 3;
  z1->dataEvent(t2);
  assert(ch == 'c');
}


int main(int argc, char* argv[]) {
  ZipTest(argc, argv);
  return 0;
}
