/*!
 * @file
 * class ReduceBuilder
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef REDUCEBUILDER_EZL_H
#define REDUCEBUILDER_EZL_H

#include <memory>

#include "ezl/helper/ProcReq.hpp"

#define RSUPER DataFlowExpr<ReduceBuilder<I, S, F, FO, O, P, H>>,                   \
               ParExpr<ReduceBuilder<I, S, F, FO, O, P, H>, P, H>,                  \
               DumpExpr<ReduceBuilder<I, S, F, FO, O, P, H>>,                       \
               ReduceExpr<ReduceBuilder<I, S, F, FO, O, P, H>, I, P, S, F, FO, O, H>\

namespace ezl {
namespace detail {

template <class T> class Source;
template <class T> struct DataFlowExpr;
template <class T, class P, class H> struct ParExpr;
template <class T> struct DumpExpr;
template <class T, class I, class P, class S, class F, class FO, class O,
          class H> struct ReduceExpr;

/*!
 * @ingroup builder
 * Builder for `Reduce` unit.
 * Employs crtp, mixin or policy based design for adding expressions with
 * nearly orthogonal functionality. Mixin helps calling super build after
 * a template class is done with its part in building the unit.
 * 
 * TODO: typelist
 * */
template <class I, class S, class F, class FO, class O, class P, class H>
struct ReduceBuilder : public RSUPER {
public:
  ReduceBuilder() = default;

  ReduceBuilder(F&& f, FO&& initVal,
                std::shared_ptr<Source<I>> prev)
      : ReduceExpr<ReduceBuilder, I, P, S, F, FO, O, H>{std::forward<F>(f), prev, std::forward<FO>(initVal)} {
    this->prll(Karta::prllRatio); 
  }

  auto& self() { return *this; }

  template <class NO>
  auto reReduceExpr(NO) {
    auto temp = ReduceBuilder<I, S, F, FO, NO, P, H>{
      std::forward<F>(this->_func), std::forward<FO>(this->_initVal), std::move(this->_prev)}; 
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  template <class NH>
  auto reHashExpr(NH) {
    auto temp = ReduceBuilder<I, S, F, FO, O, P, NH>{
      std::forward<F>(this->_func), std::forward<FO>(this->_initVal), std::move(this->_prev)}; 
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  auto build() {
    ParExpr<ReduceBuilder, P, H>::build();
    auto obj = ReduceExpr<ReduceBuilder, I, P, S, F, FO, O, H>::build();
    DumpExpr<ReduceBuilder>::buildAdd(obj);
    return obj;
  }

};
}
} // namespace ezl namespace ezl::detail

#endif // !REDUCEBUILDER_EZL_H
