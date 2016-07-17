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

#include <boost/functional/hash.hpp>

#include <ezl/mapreduce/Map.hpp>


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
 * Employs crtp
 *
 * */
template <class I, class S, class F, class O, class P, class H, class A>
struct MapBuilder : MSUPER {
public:
  MapBuilder(F &&f, std::shared_ptr<Source<I>> prev, std::shared_ptr<A> a)
      : _func{std::forward<F>(f)}, _prev{prev} {
    this->mode(llmode::shard); 
    this->_fl = a;
  }

  auto& self() { return *this; }

  template <class NO>
  auto colsSlct(NO = NO{}) {  // NOTE: while calling with T cast arg. is req 
    auto temp = MapBuilder<I, S, F, NO, P, H, A>{std::forward<F>(this->_func), std::move(this->_prev), std::move(this->_fl)};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  template <class NOP>
  auto prllSlct(NOP = NOP{}) {
    constexpr auto size = std::tuple_size<I>::value;
    using NP = typename meta::saneSlctImpl<size, NOP>::type; 
    using HN = boost::hash<typename meta::SlctTupleRefType<I, NP>::type>;
    auto temp = MapBuilder<I, S, F, O, NP, HN, A>{std::forward<F>(this->_func), std::move(this->_prev), std::move(this->_fl)};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  template <class NH>
  auto hash() {
    auto temp = MapBuilder<I, S, F, O, P, NH, A>{std::forward<F>(this->_func), std::move(this->_prev), std::move(this->_fl)};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

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

  auto buildUnit() {
    _prev = PrllExpr<MapBuilder>::preBuild(_prev, P{}, H{});
    auto obj = std::make_shared<Map<meta::MapTypes<I, S, F, O>>>(std::forward<F>(_func));
    obj->prev(_prev, obj);
    DumpExpr<MapBuilder, O>::postBuild(obj);
    return obj;
  }

  auto prev() { return _prev; }

  std::false_type isAddFirst;
private:
  F _func;
  std::shared_ptr<Source<I>> _prev;
};
}
} // namespace ezl namespace ezl::detail

#endif // !MAPBUILDER_EZL_H
