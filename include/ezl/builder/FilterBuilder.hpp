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
#ifndef FILTERBUILDER_EZL_H
#define FILTERBUILDER_EZL_H

#include <memory>

#include <boost/functional/hash.hpp>

#include <ezl/mapreduce/Filter.hpp>

#define FSUPER DataFlowExpr<FilterBuilder<I, S, F, O, P, H>>,                 \
               PrllExpr<FilterBuilder<I, S, F, O, P, H>>,                      \
               DumpExpr<FilterBuilder<I, S, F, O, P, H>, O>

namespace ezl {
namespace detail {

template <class T> class Source;
template <class T> struct DataFlowExpr;
template <class T> struct PrllExpr;
template <class T, class O> struct DumpExpr;
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

  FilterBuilder(F &&f, std::shared_ptr<Source<I>> prev)
      : _func{std::forward<F>(f)}, _prev{prev} {
    this->mode(llmode::shard);
  }

  auto& self() { return *this; }

  template <class NO>
  auto reOutputExpr(NO) {  // NOTE: while calling with T cast arg. is req
    auto temp = FilterBuilder<I, S, F, NO, P, H>{std::forward<F>(this->_func),
                                                 std::move(this->_prev)};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  template <class NOP>
  auto rePrllExpr(NOP) {
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
    _prev = PrllExpr<FilterBuilder>::build(_prev, P{}, H{});
    auto obj = std::make_shared<Filter<I, S, F, O>>(std::forward<F>(_func));
    obj->prev(_prev, obj);
    DumpExpr<FilterBuilder, O>::build(obj);
    return obj;
  }

  auto prev() { return _prev; }

private:
  F _func;
  std::shared_ptr<Source<I>> _prev;
};

}} // namespace ezl namespace ezl::detail

#endif // !FILTERBUILDER_EZL_H
