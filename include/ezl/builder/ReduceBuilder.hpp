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

#include <ezl/mapreduce/Reduce.hpp>

#define RSUPER DataFlowExpr<ReduceBuilder<I, S, F, FO, O, P, H>>,             \
               PrllExpr<ReduceBuilder<I, S, F, FO, O, P, H>>,                  \
               DumpExpr<ReduceBuilder<I, S, F, FO, O, P, H>, O>

namespace ezl {
namespace detail {

template <class T> class Source;
template <class T> struct DataFlowExpr;
template <class T> struct PrllExpr;
template <class T, class O> struct DumpExpr;

/*!
 * @ingroup builder
 * Builder for `Reduce` unit.
 * Employs crtp
 * 
 * TODO: typelist
 * */
template <class I, class S, class F, class FO, class O, class P, class H>
struct ReduceBuilder : public RSUPER {
public:
  ReduceBuilder() = default;

  ReduceBuilder(F &&f, FO &&initVal, std::shared_ptr<Source<I>> prev, bool scan)
      : _func{std::forward<F>(f)}, _prev{prev}, _initVal{std::forward<FO>(initVal)},
        _scan{scan} {
    this->prll(Karta::prllRatio); 
  }

  auto& self() { return *this; }

  template <class NO>
  auto reOutputExpr(NO) {
    auto temp = ReduceBuilder<I, S, F, FO, NO, P, H>{
        std::forward<F>(_func), std::forward<FO>(_initVal),
        std::move(_prev), _scan};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  template <class NH>
  auto reHashExpr(NH) {
    auto temp = ReduceBuilder<I, S, F, FO, O, P, NH>{
        std::forward<F>(_func), std::forward<FO>(_initVal),
        std::move(_prev), _scan};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  auto build() {
    _prev = PrllExpr<ReduceBuilder>::build(_prev, P{}, H{});
    auto ordered = this->getOrdered();
    auto obj =
        std::make_shared<Reduce<meta::ReduceTypes<I, P, S, F, FO, O>, H>>(
            std::forward<F>(_func), std::forward<FO>(_initVal), _scan, ordered);
    obj->prev(_prev, obj);
    DumpExpr<ReduceBuilder, O>::build(obj);
    return obj;
  }

  auto prev() { return _prev; }

  auto scan(bool isScan = true) {
    _scan = isScan;
    return *this;
  }

private:
  F _func;
  std::shared_ptr<Source<I>> _prev;
  FO _initVal;
  bool _scan{false};
};
}
} // namespace ezl namespace ezl::detail

#endif // !REDUCEBUILDER_EZL_H
