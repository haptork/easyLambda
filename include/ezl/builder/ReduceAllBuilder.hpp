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

#include <ezl/units/ReduceAll.hpp>

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
 *
 * Employs CRTP
 * */
template <class I, class S, class F, class O, class P, class H, class A>
struct ReduceAllBuilder : RASUPER {
public:
  ReduceAllBuilder(F &&f, std::shared_ptr<Source<I>> prev, Flow<A, std::nullptr_t> a,
                   bool adjacent, int bunchsize, bool fix = false, H &&h = H{})
      : _func{std::forward<F>(f)}, _prev{prev}, _adjacent{adjacent},
        _bunchSize{bunchsize}, _fixed{fix}, _h{std::forward<H>(h)} {
    this->prll(Karta::prllRatio);
    this->_fl = a;
  }

  // if the rows need to be sent to the user function once they
  // reach a certain number. Useful to group by ordering or viewing
  // partial results before overall
  // @param bunchSize number of rows to bunch together
  // @param fixed does the rows in the end need to be passed to user function
  //              even if they are not the same size as given in bunchSize
  auto& bunch(int bunchSize = 2, bool fixed = false) {
    if (bunchSize <= 0) {
      _bunchSize = 0;
      return *this;
    }
    _bunchSize = bunchSize;
    _adjacent = false;
    _fixed = fixed;
    return *this;
  }

  // if the rows need to be sent to the user function in sliding window
  // fashion once they reach a certain number. Useful for grouping adjacent rows
  // by ordering such as in central difference, trajectory directions etc.
  // @param bunchSize the size of the window or number of rows to bunch together
  // @param fixed whether to call when rows are lesser than window size or not as
  //              during the beginning or ending of the rows.
  auto& adjacent(int size = 2, bool fixed = false) {
    if (size <= 0) {
      _bunchSize = 0;
      _adjacent = false;
      return *this;
    }
    _bunchSize = size;
    _adjacent = true;
    _fixed = fixed;
    return *this;
  }

  /*!
   * internally called by cols and colsDrop
   * @param NO template param for selection columns 
   * @return new type after setting the output columns
   * */
  template <class NO>
  auto colsSlct(NO = NO{}) {
    auto temp = ReduceAllBuilder<I, S, F, NO, P, H, A>{
        std::forward<F>(_func), std::move(_prev), std::move(this->_fl),
        _adjacent, _bunchSize, _fixed, std::forward<H>(_h)};
    temp.prllProps(this->prllProps());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  /*!
   * set partition function for partitioning on keys for parallelism.
   * @param nh partition function that takes tuple of const ref of key column types
   *           and returns an integer for the partitioning. The default is hash function.
   * */
  template <class NH>
  auto partitionBy(NH &&nh) {
    auto temp = ReduceAllBuilder<I, S, F, O, P, NH, A>{
        std::forward<F>(_func), std::move(_prev), std::move(this->_fl),
        _adjacent, _bunchSize, _fixed, std::forward<NH>(nh)};
    temp.prllProps(this->prllProps());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  /*!
   * internally called by build
   * @return shared_ptr of ReduceAll object.
   * */
  auto _buildUnit() {
    _prev = PrllExpr<ReduceAllBuilder>::_preBuild(_prev, P{}, std::forward<H>(_h));
    auto ordered = this->getOrdered();
    auto obj = std::make_shared<ReduceAll<meta::ReduceAllTypes<I, P, S, F, O>>>(
        std::forward<F>(_func), ordered, _adjacent, _fixed, _bunchSize);
    obj->prev(_prev, obj);
    DumpExpr<ReduceAllBuilder, O>::_postBuild(obj);
    return obj;
  }

  // returns self / *this
  auto& _self() { return *this; }

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
  bool _adjacent;
  int _bunchSize;
  bool _fixed;
  H _h;
};
}
} // namespace ezl namespace ezl::detail

#endif // !REDUCEALLBUILDER_EZL_H
