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

#include <ezl/helper/meta/typeInfo.hpp>
#include <ezl/units/Filter.hpp>

#define FSUPER DataFlowExpr<FilterBuilder<I, S, F, O, P, H, A>, A>,    \
               PrllExpr<FilterBuilder<I, S, F, O, P, H, A>>,           \
               DumpExpr<FilterBuilder<I, S, F, O, P, H, A>, O>

namespace ezl {
template <class T> class Source;
namespace detail {


template <class T, class A> struct DataFlowExpr;
template <class T> struct PrllExpr;
template <class T, class O> struct DumpExpr;
/*!
 * @ingroup builder
 * Builder for `Filter class`
 *
 * Employs CRTP.
 *
 * */
template <class I, class S, class F, class O, class P, class H, class A>
struct FilterBuilder : FSUPER {
public:
  FilterBuilder(F &&f, std::shared_ptr<Source<I>> prev,
                Flow<A, std::nullptr_t> fl, H &&h = H{})
      : _func{std::forward<F>(f)}, _prev{prev}, _h{std::forward<H>(h)} {
    this->mode(llmode::shard);
    this->_fl = fl;
  }

  /*!
   * set partition with key for parallelism.
   * @param i, is... template ints e.g. partition<3,1>() sets third and first column
   *        as key for partition.
   * @param nh optional partition function that takes tuple of const ref of column types
   *           and returns an integer for the partitioning. The default is hash function.
   * */
  template <int i, int... is, class NH = meta::HashType<I, i, is...>>
  auto partition(NH &&nh = NH{}) {
    using NP = meta::saneSlct<std::tuple_size<I>::value, i, is...>;
    auto temp = FilterBuilder<I, S, F, O, NP, NH, A>{std::forward<F>(_func), 
        std::move(_prev), std::move(this->_fl), std::forward<NH>(nh)};
    this->_props.isPrll = true;
    temp.prllProps(this->prllProps());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  // returns self / *this
  auto& _self() { return *this; }

  /*!
   * internally called by cols and colsDrop
   * @param NO template param for selection columns 
   * @return new type after setting the output columns
   * */
  template <class NO>
  auto colsSlct(NO = NO{}) {  // NOTE: while calling with T cast arg. is req
    auto temp = FilterBuilder<I, S, F, NO, P, H, A>{std::forward<F>(_func), 
        std::move(_prev), std::move(this->_fl), std::forward<H>(_h)};
    temp.prllProps(this->prllProps());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  /*!
   * internally called by build
   * @return shared_ptr of Filter object.
   * */
  auto _buildUnit() {
    _prev = PrllExpr<FilterBuilder>::_preBuild(_prev, P{}, std::forward<H>(_h));
    auto obj = std::make_shared<Filter<I, S, F, O>>(std::forward<F>(_func));
    obj->prev(_prev, obj);
    DumpExpr<FilterBuilder, O>::_postBuild(obj);
    return obj;
  }

  /*!
   * returns prev unit in the dataflow
   * */
  auto prev() { return _prev; }

private:
  F _func;
  std::shared_ptr<Source<I>> _prev;
  H _h;
};

}} // namespace ezl namespace ezl::detail

#endif // !FILTERBUILDER_EZL_H
