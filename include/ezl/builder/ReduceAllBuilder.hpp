/*!
 * @file
 * class ReduceAllBuilder
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef REDUCEALLBUILDER_EZL_H
#define REDUCEALLBUILDER_EZL_H

#include <memory>

#include "ezl/helper/ProcReq.hpp"


#define RASUPER DataFlowExpr<ReduceAllBuilder<I, S, F, O, P, H>>,                   \
               ParExpr<ReduceAllBuilder<I, S, F, O, P, H>, P, H>,                  \
               DumpExpr<ReduceAllBuilder<I, S, F, O, P, H>>,                       \
               ReduceAllExpr<ReduceAllBuilder<I, S, F, O, P, H>, I, P, S, F, O, H>\


namespace ezl {
namespace detail {

template <class T> class Source;
template <class T> struct DataFlowExpr;
template <class T, class P, class H> struct ParExpr;
template <class T> struct DumpExpr;
template <class T, class I, class P, class S, class F, class O, class H> struct ReduceAllExpr;
/*!
 * @ingroup builder
 * Builder for `ReduceAll` unit.
 * Employs crtp, mixin or policy based design for adding expressions with
 * nearly orthogonal functionality. Mixin helps calling super build after
 * a template class is done with its part in building the unit.
 *
 * */
template <class I, class S, class F, class O, class P, class H>
struct ReduceAllBuilder : public RASUPER {
public:
  ReduceAllBuilder() = default;

  ReduceAllBuilder(F&& f, std::shared_ptr<Source<I>> prev, bool adj, int bunch)
      : ReduceAllExpr<ReduceAllBuilder, I, P, S, F, O, H>{std::forward<F>(f), prev, adj, bunch} {
    this->prll(Karta::prllRatio);
  }

  auto& self() { return *this; }

  template <class NO>
  auto reReduceAllExpr(NO) {
    auto temp = ReduceAllBuilder<I, S, F, NO, P, H>{
      std::forward<F>(this->_func), std::move(this->_prev), this->_adjacent, this->_bunchSize};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  template <class NH>
  auto reHashExpr(NH) {
    auto temp = ReduceAllBuilder<I, S, F, O, P, NH>{
      std::forward<F>(this->_func), std::move(this->_prev), this->_adjacent, this->_bunchSize};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  auto build() {
    ParExpr<ReduceAllBuilder, P, H>::build();
    auto obj = ReduceAllExpr<ReduceAllBuilder, I, P, S, F, O, H>::build();
    DumpExpr<ReduceAllBuilder>::buildAdd(obj);
    return obj;
  }

};
}
} // namespace ezl namespace ezl::detail

#endif // !REDUCEALLBUILDER_EZL_H
