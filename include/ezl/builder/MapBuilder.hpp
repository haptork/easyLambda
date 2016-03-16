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
#ifndef __MAPBUILDER_EZL_H__
#define __MAPBUILDER_EZL_H__

#include <memory>

#include <boost/functional/hash.hpp>

#include "ezl/helper/meta/slctTuple.hpp"
#include "ezl/helper/ProcReq.hpp"


#define MSUPER DataFlowExpr<MapBuilder<I, S, F, O, P, H>>,                   \
               ParExpr<MapBuilder<I, S, F, O, P, H>, P, H>,                  \
               DumpExpr<MapBuilder<I, S, F, O, P, H>>,                       \
               MapExpr<MapBuilder<I, S, F, O, P, H>, I, S, F, O>

namespace ezl {
namespace detail {

template <class T> class Source;
template <class T> struct DataFlowExpr;
template <class T, class O, class H> struct ParExpr;
template <class T> struct DumpExpr;
template <class T, class I, class S, class F, class O> struct MapExpr;

/*!
 * @ingroup builder
 * Builder for `Map` unit.
 * Employs crtp, mixin or policy based design for adding expressions with
 * nearly orthogonal functionality. Mixin helps calling super build after
 * a template class is done with its part in building the unit.
 *
 * */
template <class I, class S, class F, class O, class P, class H>
struct MapBuilder : public MSUPER {
public:

  MapBuilder() = default;

  MapBuilder(F&& f, std::shared_ptr<Source<I>> prev)
      : MapExpr<MapBuilder, I, S, F, O>{std::forward<F>(f), prev} {
    this->mode(llmode::shard); 
  }

  auto& self() { return *this; }

  template <class NO>
  auto reMapExpr(NO) {  // NOTE: while calling with T cast arg. is req 
    auto temp = MapBuilder<I, S, F, NO, P, H>{std::forward<F>(this->_func), std::move(this->_prev)};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  template <class NOP>
  auto reParExpr(NOP) {
    constexpr auto size = std::tuple_size<I>::value;
    using NP = typename meta::saneSlctImpl<size, NOP>::type; 
    using HN = boost::hash<typename meta::SlctTupleRefType<I, NP>::type>;
    auto temp = MapBuilder<I, S, F, O, NP, HN>{std::forward<F>(this->_func), std::move(this->_prev)};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return std::move(temp);
  }

  template <class NH>
  auto reHashExpr(NH) {
    auto temp = MapBuilder<I, S, F, O, P, NH>{std::forward<F>(this->_func), std::move(this->_prev)};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return std::move(temp);
  }

  auto build() {
    ParExpr<MapBuilder, P, H>::build();
    auto obj = MapExpr<MapBuilder, I, S, F, O>::build();
    DumpExpr<MapBuilder>::buildAdd(obj);
    return std::move(obj);
  }

};
}
} // namespace ezl namespace ezl::detail

#endif // !__MAPBUILDER_EZL_H__
