/*!
 * @file
 * class FilterBuilder
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef __FILTERBUILDER_EZL_H__
#define __FILTERBUILDER_EZL_H__

#include <memory>

#include <boost/functional/hash.hpp>

#include "ezl/helper/meta/slctTuple.hpp"
#include "ezl/helper/ProcReq.hpp"

#define FSUPER                                                                \
  DataFlowExpr<                                                               \
      FilterBuilder<I, S, F, O, P, H>>,                                       \
      ParExpr<FilterBuilder<I, S, F, O, P, H>, P, H>,                         \
      DumpExpr<FilterBuilder<I, S, F, O, P, H>>,                              \
      FilterExpr<FilterBuilder<I, S, F, O, P, H>, I, S, F, O>

namespace ezl {
namespace detail {

template <class T> class Source;
template <class T> struct DataFlowExpr;
template <class T, class O, class H> struct ParExpr;
template <class T> struct DumpExpr;
template <class T, class I, class S, class F, class O> struct FilterExpr;
/*!
 * @ingroup builder
 * Builder for `Filter class`
 *
 * Employs CRTP.
 *
 * */
template <class I, class S, class F, class O, class P, class H>
struct FilterBuilder : public FSUPER {
public:
  FilterBuilder() = default;

  FilterBuilder(F&& f, std::shared_ptr<Source<I>> prev)
      : FilterExpr<FilterBuilder, I, S, F, O>{std::forward<F>(f), prev} {
    this->mode(llmode::shard);
  }

  auto& self() { return *this; }

  template <class NO>
  auto reFilterExpr(NO) {  // NOTE: while calling with T cast arg. is req
    auto temp = FilterBuilder<I, S, F, NO, P, H>{std::forward<F>(this->_func),
                                                 std::move(this->_prev)};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  template <class NOP>
  auto reParExpr(NOP) {
    constexpr auto size = std::tuple_size<I>::value;
    using NP = typename meta::saneSlctImpl<size, NOP>::type; 
    using HN = boost::hash<typename meta::SlctTupleRefType<I, NP>::type>;
    auto temp = FilterBuilder<I, S, F, O, NP, HN>{std::move(this->_func),
                                                  std::move(this->_prev)};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  template <class NH>
  auto reHashExpr(NH) {
    auto temp = FilterBuilder<I, S, F, O, P, NH>{std::move(this->_func),
                                                 std::move(this->_prev)};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  auto build() {
    ParExpr<FilterBuilder, P, H>::build();
    auto obj = FilterExpr<FilterBuilder, I, S, F, O>::build();
    DumpExpr<FilterBuilder>::buildAdd(obj);
    return std::move(obj);
  }

};

}} // namespace ezl namespace ezl::detail

#endif // !__FILTERBUILDER_EZL_H__
