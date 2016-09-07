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

#include <ezl/units/Reduce.hpp>

#define RSUPER DataFlowExpr<ReduceBuilder<I, S, F, FO, O, P, H, A>, A>,\
               PrllExpr<ReduceBuilder<I, S, F, FO, O, P, H, A>>,       \
               DumpExpr<ReduceBuilder<I, S, F, FO, O, P, H, A>, O>

namespace ezl {
template <class T> class Source;
namespace detail {

template <class T, class A> struct DataFlowExpr;
template <class T> struct PrllExpr;
template <class T, class O> struct DumpExpr;

/*!
 * @ingroup builder
 * Builder for `Reduce` unit.
 *
 * Employs CRTP
 * */
template <class I, class S, class F, class FO, class O, class P, class H, class A>
struct ReduceBuilder : RSUPER {
public:
  ReduceBuilder(F &&f, FO &&initVal, std::shared_ptr<Source<I>> prev,
                Flow<A, std::nullptr_t> a, bool scan, H &&h = H{})
      : _func{std::forward<F>(f)}, _prev{prev}, _scan{scan},
        _initVal{std::forward<FO>(initVal)}, _h{std::forward<H>(h)} {
    this->prll(Karta::prllRatio); 
    this->_fl = a;
  }

  /*!
   * makes a reduce into a scan i.e. every updated result is also sent to the
   * next units in the dataflow. Useful in running sum, running mean etc.
   * @param isScan optional boolean
   * */
  auto scan(bool isScan = true) {
    _scan = isScan;
    return *this;
  }

  /*!
   * internally called by cols and colsDrop
   * @param NO template param for selection columns 
   * @return new type after setting the output columns
   * */
  template <class NO>
  auto colsSlct(NO = NO{}) {
    auto temp = ReduceBuilder<I, S, F, FO, NO, P, H, A>{
        std::forward<F>(_func), std::forward<FO>(_initVal), std::move(_prev),
        std::move(this->_fl), _scan, std::forward<H>(_h)};
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
    auto temp = ReduceBuilder<I, S, F, FO, O, P, NH, A>{
        std::forward<F>(_func), std::forward<FO>(_initVal), std::move(_prev),
        std::move(this->_fl), _scan, std::forward<NH>(nh)};
    temp.prllProps(this->prllProps());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  /*!
   * internally called by build
   * @return shared_ptr of Reduce object.
   * */
  auto _buildUnit() {
    _prev = PrllExpr<ReduceBuilder>::_preBuild(_prev, P{}, std::forward<H>(_h));
    auto ordered = this->getOrdered();
    auto obj =
        std::make_shared<Reduce<meta::ReduceTypes<I, P, S, F, FO, O>>>(
            std::forward<F>(_func), std::forward<FO>(_initVal), _scan, ordered);
    obj->prev(_prev, obj);
    DumpExpr<ReduceBuilder, O>::_postBuild(obj);
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
  bool _scan{false};
  FO _initVal;
  H _h;
};
}
} // namespace ezl namespace ezl::detail

#endif // !REDUCEBUILDER_EZL_H
