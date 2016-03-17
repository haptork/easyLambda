/*!
 * @file
 * Single include interface for using ezl
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef EZL_EZL_H
#define EZL_EZL_H

#include <memory>
#include <tuple>

#include <ezl/builder/DataFlowExpr.hpp>
#include <ezl/builder/LoadBuilder.hpp>
#include <ezl/builder/LoadExpr.hpp>
#include <ezl/builder/LoadUnitBuilder.hpp>
#include <ezl/builder/LoadUnitExpr.hpp>
#include <ezl/helper/meta/slct.hpp>
#include <ezl/helper/meta/slctTuple.hpp>

namespace ezl {

template <class T> class Source;

template <int... I>
using val = ezl::detail::meta::slct<I...>;

template <int... I>
using key = ezl::detail::meta::slct<I...>;

template <class I> inline auto flow(std::shared_ptr<I> pr) {
  detail::LoadUnitBuilder<I> obj;
  return obj.prev(pr);
}

template <class I> auto flow(detail::LoadUnitBuilder<I> pr) { return pr; }

template <class... Is> inline auto flow() {
  using I = typename detail::meta::SlctTupleRefType<std::tuple<Is...>>::type;
  return flow(std::shared_ptr<detail::Source<I>>{nullptr});
}

template <class F> inline auto rise(F&& sourceFunc) {
  return detail::LoadBuilder<F>{std::forward<F>(sourceFunc)};
}

} // namespace ezl

#endif // ! EZL_EZL_H
