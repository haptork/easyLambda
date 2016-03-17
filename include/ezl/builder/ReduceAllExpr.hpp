/*!
 * @file
 * class ReduceAllExpr
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef REDUCEALLEXPR_EZL_H
#define REDUCEALLEXPR_EZL_H

#include <memory>
#include <vector>

#include "ezl/helper/meta/slctTuple.hpp"
#include "ezl/helper/meta/slct.hpp"
#include "ezl/mapreduce/ReduceAll.hpp"

namespace ezl {
namespace detail {

template <typename T> class Source;

/*!
 * @ingroup builder
 * terminating expression class for `ReduceAllBuilder`
 *
 * */
template <class T, class I, class K, class S, class F, class O, class H> // UDF
struct ReduceAllExpr {

  ReduceAllExpr() = default;

  ReduceAllExpr(F&& f, std::shared_ptr<Source<I>> pr, bool adjacent,
                int bunchSize)
      : _func{std::forward<F>(f)}, _prev{pr}, _adjacent{adjacent},
        _bunchSize{bunchSize} {}

  auto build() {
    auto ordered = ((T *)this)->getOrdered();
    auto obj = std::make_shared<ReduceAll<meta::ReduceAllTypes<I, K, S, F, O>, H>>(
        std::forward<F>(_func), ordered, _adjacent, _bunchSize);
    obj->prev(_prev, obj);
    return obj;
  }

  auto prev() { return _prev; }

  auto prev(std::shared_ptr<Source<I>> p) {
    _prev = std::move(p);
    return ((T *)this)->self();
  }

  auto& bunch(int bunchSize = 2) {
    _bunchSize = bunchSize;
    _adjacent = false;
    return ((T *)this)->self();
  }

  auto& adjacent(int size = 2) {
    _bunchSize = size;
    _adjacent = true;
    return ((T *)this)->self();
  }

  template <int... Ns> auto cols() {
    return ((T *)this)->reReduceAllExpr(meta::slct<Ns...>{});
  }

  template <int... Ns> auto colsDrop() {
    using NO = typename meta::setDiff<O, meta::slct<Ns...>>::type;
    return ((T *)this)->reReduceAllExpr(NO{});
  }


  F _func;
  std::shared_ptr<Source<I>> _prev;
  bool _adjacent;
  int _bunchSize;
};
}
} // namespace ezl namespace ezl::detail

#endif //!REDUCEALLEXPR_EZL_H
