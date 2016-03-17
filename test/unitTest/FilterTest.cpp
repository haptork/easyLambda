/*!
 * @file
 * Basic tests for `Filter.hpp`
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
#include <ctorTeller.hpp>

using namespace ezl::detail;

void FilterBasicCallTest();

void FilterTest(int argc, char* argv[]) {
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator comm;
  FilterBasicCallTest();
}

void FilterBasicCallTest() {
  using std::make_tuple;
  using meta::slct;
  using std::tuple;

  auto t1 = make_tuple(3, 'c', 4.);
  char ch;
  auto f1 = [&ch](char c, int i) { 
    ch = c; 
    return i+1; 
  };
  Filter<decltype(t1), slct<2,1>, decltype(f1), slct<1,2>> r1{f1};
  r1.dataEvent(t1);
  assert(ch == 'c');
}
