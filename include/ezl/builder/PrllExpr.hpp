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
#include <ezl/mapreduce/MPIBridge.hpp>

namespace ezl {
template <class T> class Source;
namespace detail {

/*!
 * @ingroup builder
 * For adding a `MPIBridge` unit in between prev and newly built unit.
 * The expression can be used by any unit builder like `MapBuilder`
 * `ReduceBuilder` etc.
 *
 * */
struct ParProps {
  bool isPrll{false};
  llmode mode {llmode::none};
  ProcReq procReq{};
  bool ordered{false};
};

template <class T> struct PrllExpr {
public:
  PrllExpr() = default;

  PrllExpr(bool isP, llmode md, ProcReq pr, bool ordered)
      : _props{isP, md, pr, ordered} {}

  auto& props() { return _props; }

  auto& props(ParProps p) { 
    _props = p;
    return ((T *)this)->self();
  }

  auto& mode(llmode md) {
    _props.mode = md;
    return ((T *)this)->self();
  }

  template <int I, int... Is, class Ptype>
  auto prll(Ptype n, llmode mode = llmode::none) {
    _props.procReq = ProcReq{n};
    _props.isPrll = true;
    _props.mode = mode;
    return ((T *)this)->prllSlct(meta::slct<I, Is...>{});
  }

  template <int I, int... Is> auto prll(llmode mode = llmode::none) {
    _props.procReq = ProcReq{};
    _props.isPrll = true;
    _props.mode = mode;
    return ((T *)this)->prllSlct(meta::slct<I, Is...>{});
  }

  template <class Ptype> auto& prll(Ptype n, llmode mode = llmode::none) {
    _props.procReq = ProcReq(n);
    _props.isPrll = true;
    _props.mode = mode;
    return ((T *)this)->self();
  }

  auto& prll(llmode mode = llmode::none) {
    _props.procReq = ProcReq{};
    _props.isPrll = true;
    _props.mode = mode;
    return ((T *)this)->self();
  }

  template <int I, int... Is>
  auto prll(std::initializer_list<int> lprocs, llmode mode = llmode::none) {
    auto procs = std::vector<int>(std::begin(lprocs), std::end(lprocs));
    _props.procReq = ProcReq{procs};
    _props.isPrll = true;
    _props.mode = mode;
    return ((T *)this)->prllSlct(meta::slct<I, Is...>{});
  }

  auto& prll(std::initializer_list<int> lprocs, llmode mode = llmode::none) {
    auto procs = std::vector<int>(std::begin(lprocs), std::end(lprocs));
    _props.procReq = ProcReq{procs};
    _props.isPrll = true;
    _props.mode = mode;
    return ((T *)this)->self();
  }

  auto ordered(bool flag = true) {
    _props.ordered = flag;
    return ((T *)this)->self();
  }

  auto& getOrdered(bool flag = true) {
    return _props.ordered;
  }

  auto& inprocess() {
    _props.isPrll = false;
    return ((T *)this)->self();
  }

protected:
  template <class I, class P, class H>
  auto preBuild(std::shared_ptr<Source<I>> pre, P, H) {
    if (_props.isPrll) {
      if (P::size == 0 && !(_props.mode & llmode::shard || _props.mode & llmode::all)) {
        _props.procReq.resize(1);
      }
      bool all = false;
      if (P::size == 0 && (_props.mode & llmode::all)) all = true;
      if (_props.mode & llmode::task) _props.procReq.setTask();
      auto bobj = std::make_shared<MPIBridge<I, P, H>>(
          _props.procReq, all, _props.ordered);
      bobj->prev(pre, bobj);
      pre = bobj;
      return pre;
    }
    return pre;
  }

private:
  ParProps _props;
};
}
} // namespace ezl namespace ezl::detail

#endif
