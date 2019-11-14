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

#ifndef NOMPI
#include <boost/mpi.hpp>
#endif

#include <memory>
#include <tuple>

#include <ezl/builder/DataFlowExpr.hpp>
#include <ezl/builder/RiseBuilder.hpp>
#include <ezl/builder/LoadUnitBuilder.hpp>
#include <ezl/helper/meta/slct.hpp>
#include <ezl/helper/meta/slctTuple.hpp>
#include <ezl/units/NoOp.hpp>

namespace ezl {

template <class T> class Source;
template <int... I>
using val = detail::meta::slct<I...>;
template <int... I>
using key = detail::meta::slct<I...>;
// start a dataflow from a vector of prior dataflow or units
template <class I> inline auto flow(std::vector<std::shared_ptr<I>> vpr) {
  auto fl = Flow<typename I::otype, std::nullptr_t>{};
  auto noop = std::make_shared<NoOp<typename I::otype>>();
  fl.addFirst(noop);
  for (auto& pr : vpr) if (pr) pr->next(noop, pr);
  return detail::LoadUnitBuilder<NoOp<typename I::otype>, typename I::otype> {noop, fl};
}
// start a dataflow from a list of prior dataflow or units
template <class I> inline auto flow(std::initializer_list<std::shared_ptr<I>> vpr) {
  return flow(std::vector<std::shared_ptr<I>> (vpr.begin(), vpr.end()));
}
// start a dataflow from prior dataflow or units
template <class I> inline auto flow(std::shared_ptr<I> pr) {
  auto fl = Flow<typename I::otype, std::nullptr_t>{};
  auto noop = std::make_shared<NoOp<typename I::otype>>();
  if (pr) pr->next(noop, pr);
  fl.addFirst(noop);
  return detail::LoadUnitBuilder<NoOp<typename I::otype>, typename I::otype> {noop, fl};
}
// start a dataflow without rise by mentioning type of input stream
template <class... Is> inline auto flow() {
  using I = typename detail::meta::SlctTupleRefType<std::tuple<Is...>>::type;
  return flow(std::shared_ptr<Source<I>>{nullptr});
}
// start a dataflow with a rise
template <class F> inline auto rise(F&& sourceFunc) {
  return detail::RiseBuilder<F>{std::forward<F>(sourceFunc)};
}
} // namespace ezl

#endif // ! EZL_EZL_H
