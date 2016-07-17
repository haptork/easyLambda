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
#include <ezl/builder/RiseBuilder.hpp>
#include <ezl/builder/LoadUnitBuilder.hpp>
#include <ezl/helper/meta/slct.hpp>
#include <ezl/helper/meta/slctTuple.hpp>

namespace ezl {

template <class T> class Source;

template <int... I>
using val = ezl::detail::meta::slct<I...>;

template <int... I>
using key = ezl::detail::meta::slct<I...>;

template <class I> inline auto flow(std::vector<std::shared_ptr<I>> vpr) {
  using Fl = Flow<typename I::otype, std::false_type>;
  auto fl = std::make_shared<Fl>();
  for (auto& pr : vpr) fl->flprevSet(pr);
  return detail::LoadUnitBuilder<I, std::true_type, Fl> {vpr[0], fl};
}

template <class I> inline auto flow(std::initializer_list<std::shared_ptr<I>> vpr) {
  return flow(std::vector<std::shared_ptr<I>> (vpr.begin(), vpr.end()));
}

template <class I> inline auto flow(std::shared_ptr<I> pr) {
  return detail::LoadUnitBuilder<I, std::true_type, Flow<typename I::otype, std::false_type>> {pr};
}

template <class... Is> inline auto flow() {
  using I = typename detail::meta::SlctTupleRefType<std::tuple<Is...>>::type;
  return flow(std::shared_ptr<Source<I>>{nullptr});
}

template <class F> inline auto rise(F&& sourceFunc) {
  return detail::RiseBuilder<F, Flow<int, std::false_type>>{
      std::forward<F>(sourceFunc)};
}

} // namespace ezl

#endif // ! EZL_EZL_H
