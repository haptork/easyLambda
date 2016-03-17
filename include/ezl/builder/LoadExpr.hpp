/*!
 * @file
 * Interface for client apps
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef LOADEXPR__EZL__H
#define LOADEXPR__EZL__H

#include <initializer_list>
#include <map>
#include <memory>
#include <vector>

#include "ezl/helper/ProcReq.hpp"
#include "ezl/mapreduce/Load.hpp"

namespace ezl {
namespace detail {

/*!
 * @ingroup builder
 * Expression terminator for `LoadBuilder`.
 * Provides expressions for building `Load` unit.
 *
 * */
template <class T, class F> struct LoadExpr {

  LoadExpr(F&& sourceFunc) : _sourceFunc{std::forward<F>(sourceFunc)}, _procReq{} {}

  auto build() {
    //decltype(_sourceFunc) a = 5;
    return std::make_shared<Load<F>>(_procReq, std::forward<F>(_sourceFunc), _procSink);
  }

  auto& procDump(std::pair<int, std::vector<int>> &procSink) {
    _procSink = &procSink;
    return ((T *)this)->self();
  }

  template <class Ptype> auto& prll(Ptype p) {
    _procReq = ProcReq(p);
    return ((T *)this)->self();
  }

  auto& prll() {
    _procReq = ProcReq{};
    return ((T *)this)->self();
  }

  auto& prll(std::initializer_list<int> l) {
    auto procs = std::vector<int>(std::begin(l), std::end(l));
    _procReq = ProcReq{procs};
    return ((T *)this)->self();
  }

  auto& noprll() {
    _procReq = ProcReq(1);
    return ((T *)this)->self();
  }

  F _sourceFunc;
  ProcReq _procReq;
  std::pair<int, std::vector<int>> *_procSink{nullptr};
};
}
} // namespace ezl namespace ezl::detail

#endif
