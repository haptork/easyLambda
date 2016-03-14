/*!
 * @file
 * class ReduceExpr
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef __REDUCEEXPR_EZL_H__
#define __REDUCEEXPR_EZL_H__

#include <memory>
#include <vector>
#include <type_traits>

#include "ezl/mapreduce/Reduce.hpp"
#include "ezl/helper/meta/slctTuple.hpp"
#include "ezl/helper/meta/slct.hpp"
#include "ezl/helper/meta/typeInfo.hpp"

namespace ezl {
namespace detail {

template <typename T> class Source;

/*!
 * @ingroup builder
 * Terminating expression class for `ReduceBuilder`
 * TODO: typelist
 * */
template <class T, class I, class K, class S, class F, class FO, class O, class H> // UDF
struct ReduceExpr {

  ReduceExpr() = default;

  ReduceExpr(F&& f, std::shared_ptr<Source<I>> pr, FO&& i)
      : _func{std::forward<F>(f)}, _prev{pr}, _initVal{std::forward<FO>(i)} {}

  auto build() {
    auto ordered = ((T *)this)->getOrdered();
    auto obj =
        std::make_shared<Reduce<meta::ReduceTypes<I, K, S, F, FO, O>, H>>(
            std::forward<F>(_func), std::forward<FO>(_initVal), ordered);
    obj->prev(_prev, obj);
    return std::move(obj);
  }

  auto prev() { return _prev; }

  auto& prev(std::shared_ptr<Source<I>> p) {
    _prev = std::move(p);
    return ((T *)this)->self();
  }

  template <int... Ns> auto cols() {
    return ((T *)this)->reReduceExpr(meta::slct<Ns...>{});
  }

  template <int... Ns> auto colsDrop() {
    using NO = typename meta::setDiff<O, meta::slct<Ns...>>::type;
    return ((T *)this)->reReduceExpr(NO{});
  }

  F _func;
  std::shared_ptr<Source<I>> _prev;
  FO _initVal;
};
}
} // namespace ezl namespace ezl::detail

#endif
