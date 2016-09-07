/*!
 * @file
 * class MapBuilder
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef MAPBUILDER_EZL_H
#define MAPBUILDER_EZL_H

#include <memory>

#include <ezl/helper/meta/typeInfo.hpp>
#include <ezl/units/Map.hpp>


#define MSUPER DataFlowExpr<MapBuilder<I, S, F, O, P, H, A>, A>,       \
               PrllExpr<MapBuilder<I, S, F, O, P, H, A>>,              \
               DumpExpr<MapBuilder<I, S, F, O, P, H, A>, O>

namespace ezl {
template <class T> class Source;
namespace detail {

template <class T, class A> struct DataFlowExpr;
template <class T> struct PrllExpr;
template <class T, class O> struct DumpExpr;

/*!
 * @ingroup builder
 * Builder for `Map` unit.
 *
 * Employs CRTP
 * */
template <class I, class S, class F, class O, class P, class H, class A>
struct MapBuilder : MSUPER {
public:
  MapBuilder(F &&f, std::shared_ptr<Source<I>> prev, Flow<A, std::nullptr_t> fl,
             H h = H{})
      : _func{std::forward<F>(f)}, _prev{prev}, _h{std::forward<H>(h)} {
    this->mode(llmode::shard); 
    this->_fl = fl;
  }

  /*!
   * set partition with key for parallelism.
   * @param i, is... template ints e.g. partitionBy<3,1>() sets third and first column
   *        as key for partition.
   * @param nh optional partition function that takes tuple of const ref of key column types
   *           and returns an integer for the partitioning. The default is hash function.
   * */
  template <int i, int... is, class NH = meta::HashType<I, i, is...>>
  auto partitionBy(NH &&nh = NH{}) {
    using NP = meta::saneSlct<std::tuple_size<I>::value, i, is...>;
    auto temp = MapBuilder<I, S, F, O, NP, NH, A>{std::forward<F>(this->_func),
        std::move(this->_prev), std::move(this->_fl), std::forward<NH>(nh)};
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
    auto temp = MapBuilder<I, S, F, NO, P, H, A>{std::forward<F>(this->_func),
        std::move(this->_prev), std::move(this->_fl), std::forward<H>(this->_h)};
    temp.prllProps(this->prllProps());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  /*!
   * sets the output columns such that the selected input columns
   * are replaced by columns in function result in-place.
   * @return new type after setting the output columns accordingly
   * */
  auto colsTransform() {
    using finptype = typename meta::SlctTupleRefType<I, S>::type;
    using fouttype = typename meta::GetTupleType<decltype(meta::invokeMap(
      std::declval<typename std::add_lvalue_reference<F>::type>(),
      std::declval<typename std::add_lvalue_reference<finptype>::type>()))>::
      type;
    using NO = typename meta::inPlace<std::tuple_size<I>::value, 
          std::tuple_size<fouttype>::value, S>::type;
    return colsSlct(NO{});
  }

  /*!
   * sets the output columns such that only the function result columns
   * are selected for output.
   * @return new type after setting the output columns accordingly
   * */
  auto colsResult() {
    using finptype = typename meta::SlctTupleRefType<I, S>::type;
    constexpr auto foutsize =
        std::tuple_size<typename meta::GetTupleType<decltype(meta::invokeMap(
            std::declval<typename std::add_lvalue_reference<F>::type>(),
            std::declval<typename std::add_lvalue_reference<
                finptype>::type>()))>::type>::value;
    constexpr auto isize = std::tuple_size<I>::value;
    using NO = typename meta::fillSlct<isize, isize + foutsize>::type;
    return colsSlct(NO{});
  }

  /*!
   * internally called by build
   * @return shared_ptr of Filter object.
   * */
  auto _buildUnit() {
    _prev = PrllExpr<MapBuilder>::_preBuild(_prev, P{}, std::forward<H>(_h));
    auto obj = std::make_shared<Map<meta::MapTypes<I, S, F, O>>>(std::forward<F>(_func));
    obj->prev(_prev, obj);
    DumpExpr<MapBuilder, O>::_postBuild(obj);
    return obj;
  }

  /*!
   * returns prev unit in the dataflow
   * */
  auto prev() { return _prev; }

  /*!
   * internally used to signal if the next unit will be the first unit in the
   * dataflow. Usually of false_type.
   * */
  std::false_type _isAddFirst;
private:
  F _func;
  std::shared_ptr<Source<I>> _prev;
  H _h;
};
}
} // namespace ezl namespace ezl::detail

#endif // !MAPBUILDER_EZL_H
