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
#include <ezl/units/Rise.hpp>

#define SUPERLA DataFlowExpr<RiseBuilder<F>, std::nullptr_t>,                    \
                DumpExpr<RiseBuilder<F>, meta::slct<>>

namespace ezl {
namespace detail {

template <class T, class Fl> struct DataFlowExpr;
template <class T, class O> struct DumpExpr;

/*!
 * @ingroup builder
 * Builder for Rise
 *
 * */
template <class F> struct RiseBuilder : SUPERLA {
public:
  RiseBuilder(F&& sourceFunc) : _sourceFunc{std::forward<F>(sourceFunc)}, _procReq{} {}

  /*!
   * set a sink variable to pass the process information before the dataflow
   * executes. Does not take ownership of the variable.
   * @param procSink variable to update with parallel information 
   * */
  auto& procDump(std::pair<int, std::vector<int>> &procSink) {
    _procSink = &procSink;
    return *this;
  }

  /*!
   * parallel request for the rise to run on.
   * @param procReq Process request in terms of exact number of processes, ratio
   *        of total processes wrt to dataflow or exact rank of processes in a 
   *        vector<int>.
   * */
  template <class Ptype> auto& prll(Ptype p) {
    _procReq = ProcReq(p);
    return *this;
  }

  /*!
   * parallel with the default request i.e. all the processes in dataflow
   * */
  auto& prll() {
    _procReq = ProcReq{};
    return *this;
  }

  /*!
   * parallel request for the rise to run on.
   * @param lprocs process request as exact ranks in initializer_list<int>
   * */
  auto& prll(std::initializer_list<int> l) {
    auto procs = std::vector<int>(std::begin(l), std::end(l));
    _procReq = ProcReq{procs};
    return *this;
  }

  /*!
   * set rise to run on single process.
   * */
  auto& noprll() {
    _procReq = ProcReq(1);
    return *this;
  }


  /*!
   * internally called by build
   * @return shared_ptr of Rise object.
   * */
  auto _buildUnit() {
    auto obj = std::make_shared<Rise<F>>(_procReq, std::forward<F>(_sourceFunc), _procSink);
    DumpExpr<RiseBuilder, meta::slct<>>::_postBuild(obj);
    return obj;
  }

  // returns self / *this
  auto& _self() { return *this; }

private:
  F _sourceFunc;
  ProcReq _procReq;
  std::pair<int, std::vector<int>> *_procSink{nullptr};
};
}
} // namespace ezl namespace ezl::detail

#endif //LOADBUILDER_EZL_H
