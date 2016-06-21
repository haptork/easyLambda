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

#include <ezl/mapreduce/ReduceAll.hpp>

#define RASUPER DataFlowExpr<ReduceAllBuilder<I, S, F, O, P, H>>,             \
                PrllExpr<ReduceAllBuilder<I, S, F, O, P, H>>,                  \
                DumpExpr<ReduceAllBuilder<I, S, F, O, P, H>, O>


namespace ezl {
namespace detail {

template <class T> class Source;
template <class T> struct DataFlowExpr;
template <class T> struct PrllExpr;
template <class T, class O> struct DumpExpr;
/*!
 * @ingroup builder
 * Builder for `ReduceAll` unit.
 * Employs crtp
 *
 * */
template <class I, class S, class F, class O, class P, class H>
struct ReduceAllBuilder : public RASUPER {
public:
  ReduceAllBuilder() = default;

  ReduceAllBuilder(F &&f, std::shared_ptr<Source<I>> prev, bool adjacent,
                   int bunchsize)
      : _func{std::forward<F>(f)}, _prev{prev}, _adjacent{adjacent},
        _bunchSize{bunchsize} {
    this->prll(Karta::prllRatio);
  }

  auto& self() { return *this; }

  template <class NO>
  auto reOutputExpr(NO) {
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

  auto& bunch(int bunchSize = 2) {
    _bunchSize = bunchSize;
    _adjacent = false;
    return *this;
  }

  auto& adjacent(int size = 2) {
    _bunchSize = size;
    _adjacent = true;
    return *this;
  }

  auto build() {
    _prev = PrllExpr<ReduceAllBuilder>::build(_prev, P{}, H{});
    auto ordered = this->getOrdered();
    auto obj = std::make_shared<ReduceAll<meta::ReduceAllTypes<I, P, S, F, O>, H>>(
        std::forward<F>(_func), ordered, _adjacent, _bunchSize);
    obj->prev(_prev, obj);
    DumpExpr<ReduceAllBuilder, O>::build(obj);
    return obj;
  }

  auto prev() { return _prev; }

private:
  F _func;
  std::shared_ptr<Source<I>> _prev;
  bool _adjacent;
  int _bunchSize;
};
}
} // namespace ezl namespace ezl::detail

#endif // !REDUCEALLBUILDER_EZL_H
