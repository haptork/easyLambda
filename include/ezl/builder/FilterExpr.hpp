/*!
 * @file
 * class FilterExpr
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef __FILTEREXPR_EZL_H__
#define __FILTEREXPR_EZL_H__

#include <memory>

#include "ezl/pipeline/Source.hpp"
#include "ezl/helper/meta/slct.hpp"
#include "ezl/helper/meta/typeInfo.hpp"
#include "ezl/mapreduce/Filter.hpp"

namespace ezl {
namespace detail {

/*!
 * @ingroup builder
 * Terminating expression class for `Filter` class builder
 * expressions for building a `Filter` class unit.
 *
 * */
template <class T, class I, class S, class F, class O> // UDF
struct FilterExpr {

  using itype = I;

  FilterExpr() = default;

  FilterExpr(F &&f, std::shared_ptr<Source<I>> pr)
      : _func{std::forward<F>(f)}, _prev{pr} {}

  auto build() {
    auto obj = std::make_shared<Filter<I, S, F, O>>(std::forward<F>(_func));
    obj->prev(_prev, obj);
    return obj;
  }

  auto prev() { return _prev; }

  auto& prev(std::shared_ptr<Source<I>> p) {
    _prev = std::move(p);
    return ((T *)this)->self();
  }

  template <int... Os> auto cols() {
    return ((T *)this)->reFilterExpr(meta::slct<Os...>{});
  }

  template <int... Ns>
  auto colsDrop() {
    using NO = typename meta::setDiff<O, meta::slct<Ns...>>::type;
    return ((T *)this)->reFilterExpr(NO{});
  }

  F _func;
  std::shared_ptr<Source<I>> _prev;
};
}
} // namespace ezl namespace ezl::detail

#endif // !__FILTEREXPR_EZL_H__
