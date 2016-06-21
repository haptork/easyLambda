/*!
 * @file
 * class RiseBuilder
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef LOADBUILDER_EZL_H
#define LOADBUILDER_EZL_H

#include <initializer_list>
#include <map>
#include <memory>
#include <vector>

#include <ezl/helper/ProcReq.hpp>
#include <ezl/mapreduce/Load.hpp>

#define SUPERLA DataFlowExpr<RiseBuilder<F>>,     \
                DumpExpr<RiseBuilder<F>, meta::slct<>>

namespace ezl {
namespace detail {

template <class T> struct DataFlowExpr;
template <class T, class O> struct DumpExpr;

/*!
 * @ingroup builder
 * Builder for Load
 *
 * */
template <class F> struct RiseBuilder : public SUPERLA {
public:
  RiseBuilder(F&& sourceFunc) : _sourceFunc{std::forward<F>(sourceFunc)}, _procReq{} {}

  auto& self() { return *this; }

  auto build() {
    auto obj = std::make_shared<Load<F>>(_procReq, std::forward<F>(_sourceFunc), _procSink);
    DumpExpr<RiseBuilder, meta::slct<>>::build(obj);
    return obj;
  }

  auto& procDump(std::pair<int, std::vector<int>> &procSink) {
    _procSink = &procSink;
    return *this;
  }

  template <class Ptype> auto& prll(Ptype p) {
    _procReq = ProcReq(p);
    return *this;
  }

  auto& prll() {
    _procReq = ProcReq{};
    return *this;
  }

  auto& prll(std::initializer_list<int> l) {
    auto procs = std::vector<int>(std::begin(l), std::end(l));
    _procReq = ProcReq{procs};
    return *this;
  }

  auto& noprll() {
    _procReq = ProcReq(1);
    return *this;
  }

  // auto prev() { return nullptr; } // TODO: check
private:
  F _sourceFunc;
  ProcReq _procReq;
  std::pair<int, std::vector<int>> *_procSink{nullptr};
};
}
} // namespace ezl namespace ezl::detail

#endif //LOADBUILDER_EZL_H
