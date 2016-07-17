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

#define RASUPER DataFlowExpr<ReduceAllBuilder<I, S, F, O, P, H, A>, A>,\
                PrllExpr<ReduceAllBuilder<I, S, F, O, P, H, A>>,       \
                DumpExpr<ReduceAllBuilder<I, S, F, O, P, H, A>, O>


namespace ezl {
template <class T> class Source;
namespace detail {

template <class T, class A> struct DataFlowExpr;
template <class T> struct PrllExpr;
template <class T, class O> struct DumpExpr;
/*!
 * @ingroup builder
 * Builder for `ReduceAll` unit.
 * Employs crtp
 *
 * */
template <class I, class S, class F, class O, class P, class H, class A>
struct ReduceAllBuilder : RASUPER {
public:
  ReduceAllBuilder(F &&f, std::shared_ptr<Source<I>> prev, std::shared_ptr<A> a,
                   bool adjacent, int bunchsize)
      : _func{std::forward<F>(f)}, _prev{prev}, _adjacent{adjacent},
        _bunchSize{bunchsize} {
    this->prll(Karta::prllRatio);
    this->_fl = a;
  }

  auto& bunch(int bunchSize = 2) {
    if (bunchSize <= 0) {
      _bunchSize = 0;
      return *this;
    }
    _bunchSize = bunchSize;
    _adjacent = false;
    return *this;
  }

  auto& adjacent(int size = 2) {
    if (size <= 0) {
      _bunchSize = 0;
      _adjacent = false;
      return *this;
    }
    _bunchSize = size;
    _adjacent = true;
    return *this;
  }

  auto& self() { return *this; }

  template <class NO>
  auto colsSlct(NO = NO{}) {
    auto temp = ReduceAllBuilder<I, S, F, NO, P, H, A>{
      std::forward<F>(this->_func), std::move(this->_prev), std::move(this->_fl), this->_adjacent, this->_bunchSize};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  template <class NH>
  auto hash() {
    auto temp = ReduceAllBuilder<I, S, F, O, P, NH, A>{
      std::forward<F>(this->_func), std::move(this->_prev), std::move(this->_fl), this->_adjacent, this->_bunchSize};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  auto buildUnit() {
    _prev = PrllExpr<ReduceAllBuilder>::preBuild(_prev, P{}, H{});
    auto ordered = this->getOrdered();
    auto obj = std::make_shared<ReduceAll<meta::ReduceAllTypes<I, P, S, F, O>, H>>(
        std::forward<F>(_func), ordered, _adjacent, _bunchSize);
    obj->prev(_prev, obj);
    DumpExpr<ReduceAllBuilder, O>::postBuild(obj);
    return obj;
  }

  auto prev() { return _prev; }

  std::false_type isAddFirst;
private:
  F _func;
  std::shared_ptr<Source<I>> _prev;
  bool _adjacent;
  int _bunchSize;
};
}
} // namespace ezl namespace ezl::detail

#endif // !REDUCEALLBUILDER_EZL_H
