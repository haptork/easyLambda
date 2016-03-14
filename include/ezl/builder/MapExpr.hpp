/*!
 * @file
 * class MapExpr
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef __MAPEXPR_EZL_H__
#define __MAPEXPR_EZL_H__

#include <memory>

#include "ezl/helper/meta/slct.hpp"
#include "ezl/helper/meta/typeInfo.hpp"
#include "ezl/helper/meta/slctTuple.hpp"
#include "ezl/mapreduce/Map.hpp"

namespace ezl {
namespace detail {

template <class T> class Source;

/*!
 * @ingroup builder
 * Terminating expression class for `MapBuilder`
 *
 * */
template <class T, class I, class S, class F, class O> // UDF
struct MapExpr {

  MapExpr() = default;

  MapExpr(F &&f, std::shared_ptr<Source<I>> pr) : _func{std::forward<F>(f)}, _prev{pr} {}

  auto build() {
    auto obj = std::make_shared<Map<meta::MapTypes<I, S, F, O>>>(std::forward<F>(_func));
    obj->prev(_prev, obj);
    return std::move(obj);
  }

  auto prev() { return _prev; }

  auto& prev(std::shared_ptr<Source<I>> p) {
    _prev = std::move(p);
    return ((T *)this)->self();
  }

  template <int... Ns> auto cols() {
    return ((T *)this)->reMapExpr(meta::slct<Ns...>{});
  }

  template <int... Ns>
  auto colsDrop() {
    using NO = typename meta::setDiff<O, meta::slct<Ns...>>::type;
    return ((T *)this)->reMapExpr(NO{});
  }

  auto colsTransform() {
    using finptype = typename meta::SlctTupleRefType<I, S>::type;
    using fouttype = typename meta::GetTupleType<decltype(meta::invokeMap(
      std::declval<typename std::add_lvalue_reference<F>::type>(),
      std::declval<typename std::add_lvalue_reference<finptype>::type>()))>::
      type;
    using NO = typename meta::inPlace<std::tuple_size<I>::value, 
          std::tuple_size<fouttype>::value, S>::type;
    return ((T *)this)->reMapExpr(NO{});
  }

  auto colsResult() {
    using finptype = typename meta::SlctTupleRefType<I, S>::type;
    constexpr auto foutsize =
        std::tuple_size<typename meta::GetTupleType<decltype(meta::invokeMap(
            std::declval<typename std::add_lvalue_reference<F>::type>(),
            std::declval<typename std::add_lvalue_reference<
                finptype>::type>()))>::type>::value;
    constexpr auto isize = std::tuple_size<I>::value;
    using NO = typename meta::fillSlct<isize, isize + foutsize>::type;
    return ((T *)this)->reMapExpr(NO{});
  }
// TODO: private
  F _func;
  std::shared_ptr<Source<I>> _prev;
};

}} // namespace ezl namespace ezl::detail

#endif // !__MAPEXPR_EZL_H__
