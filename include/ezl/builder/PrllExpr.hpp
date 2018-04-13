/*!
 * @file
 * class PrllExpr
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef PAREXPR_EZL_H
#define PAREXPR_EZL_H

#include <initializer_list>
#include <memory>
#include <vector>

#include <ezl/helper/meta/slct.hpp>
#include <ezl/helper/ProcReq.hpp>
#include <ezl/units/MPIBridge.hpp>

namespace ezl {
template <class T> class Source;
namespace detail {

// structure for parallel expression properties
struct ParProps {
  bool isPrll{false};
  llmode mode {llmode::none};
  ProcReq procReq{};
  bool ordered{false};
};


/*!
 * @ingroup builder
 * For adding a `MPIBridge` unit in between prev and newly built unit.
 * The expression can be used by any unit builder like `MapBuilder`
 * `ReduceBuilder` etc. Internally used by builders.
 *
 * */
template <class T> struct PrllExpr {
public:
  PrllExpr() = default;

  PrllExpr(bool isP, llmode md, ProcReq pr, bool ordered)
      : _props{isP, md, pr, ordered} {}

  // get the parallel properties
  auto& prllProps() { return _props; }

  // set the parallel properties
  auto& prllProps(ParProps p) { 
    _props = p;
    return ((T *)this)->_self();
  }

  // set unit as inprocess or non parallel
  auto& inprocess() {
    _props.isPrll = false;
    return ((T *)this)->_self();
  }

  // changes the parallel mode, does not makes the unit parallel
  auto& mode(llmode md) {
    _props.mode = md;
    return ((T *)this)->_self();
  }

  // makes the unit parallel with a parallel request
  // @param procReq Process request in terms of exact number of processes, ratio
  //        of total processes wrt to parent or exact rank of processes in a 
  //        vector<int>.
  // @param mode optional can be task, onAll, shard or none (default).
  template <class Ptype> auto& prll(Ptype procReq, llmode mode = llmode::none) {
    _props.procReq = ProcReq{procReq};
    _props.isPrll = true;
    if (mode != llmode::none) _props.mode = mode;
    return ((T *)this)->_self();
  }

  // makes the unit parallel with an optional mode parameter
  // @param mode optional can be llmode::task, onAll, shard or none (default).
  auto& prll(llmode mode = llmode::none) {
    _props.procReq = ProcReq{};
    _props.isPrll = true;
    if (mode != llmode::none) _props.mode = mode;
    return ((T *)this)->_self();
  }

  // makes the unit parallel with a parallel request
  // @param lprocs process request as exact ranks in initializer_list<int>
  // @param mode optional can be task, onAll, shard or none (default).
  auto& prll(std::initializer_list<int> lprocs, llmode mode = llmode::none) {
    auto procs = std::vector<int>(std::begin(lprocs), std::end(lprocs));
    _props.procReq = ProcReq{procs};
    _props.isPrll = true;
    if (mode != llmode::none) _props.mode = mode;
    return ((T *)this)->_self();
  }

  // if the parallelism is by key partitioning then is the input rows ordered on
  // keys. If set then tries to send the rows to the other process at once so as
  // to keep the order intact in the unit.
  auto ordered(bool flag = true) {
    _props.ordered = flag;
    return ((T *)this)->_self();
  }

  // get if ordered is set
  auto& getOrdered(bool flag = true) {
    return _props.ordered;
  }

protected:
  template <class I, class P, class H>
  auto _preBuild(std::shared_ptr<Source<I>> pre, P, H&& h, bool storeLast = false) {
    if (_props.isPrll) {
      if (P::size == 0 && (_props.mode == llmode::none)) {
        _props.procReq.resize(1);
      }
      bool all = false;
      if (P::size == 0 && (_props.mode == llmode::dupe)) all = true;
      if (_props.mode == llmode::task) _props.procReq.setTask();
      auto bobj = std::make_shared<MPIBridge<I, P, H>>(
          _props.procReq, all, _props.ordered, std::forward<H>(h), _last);
      if(storeLast) _last = bobj.get();
      bobj->prev(pre, bobj);
      pre = bobj;
      return pre;
    }
    return pre;
  }

  ParProps _props;
  Task* _last = nullptr;
};
}
} // namespace ezl namespace ezl::detail

#endif
