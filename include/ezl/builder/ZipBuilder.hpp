/*!
 * @file
 * class ZipBuilder
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef ZIPBUILDER_EZL_H
#define ZIPBUILDER_EZL_H

#include <memory>

#include <boost/functional/hash.hpp>

#include <ezl/units/Zip.hpp>


#define ZSUPER DataFlowExpr<ZipBuilder<I1, I2, K1, K2, O, H, A>, A>,   \
               PrllExpr<ZipBuilder<I1, I2, K1, K2, O, H, A>>,          \
               DumpExpr<ZipBuilder<I1, I2, K1, K2, O, H, A>, O>

namespace ezl {
template <class T> class Source;
namespace detail {

template <class T, class A> struct DataFlowExpr;
template <class T> struct PrllExpr;
template <class T, class O> struct DumpExpr;

/*!
 * @ingroup builder
 * Builder for `Zip` unit.
 *
 * Employs CRTP
 * */
template <class I1, class I2, class K1, class K2, class O, class H, class A>
struct ZipBuilder : ZSUPER {
public:
  ZipBuilder(std::shared_ptr<Source<I1>> prev1,
             std::shared_ptr<Source<I2>> prev2, Flow<A, std::nullptr_t> a, H &&h = H{})
      : _prev1{prev1}, _prev2{prev2}, _h{std::forward<H>(h)} {
    this->prll(Karta::prllRatio);
    this->_fl = a;
  }

  /*!
   * internally called by cols and colsDrop
   * @param NO template param for selection columns 
   * @return new type after setting the output columns
   * */
  template <class NO>
  auto colsSlct(NO = NO{}) {  // NOTE: while calling with T cast arg. is req
    auto temp = ZipBuilder<I1, I2, K1, K2, NO, H, A>{
        std::move(this->_prev1), std::move(this->_prev2), std::move(this->_fl),
        std::forward<H>(_h)};
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
    auto temp = ZipBuilder<I1, I2, K1, K2, O, NH, A>{
        std::move(this->_prev1), std::move(this->_prev2), std::move(this->_fl),
        std::forward<NH>(nh)};
    temp.prllProps(this->prllProps());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  /*!
   * internally called by build
   * @return shared_ptr of Zip object.
   * */
  auto _buildUnit() {
    _prev1 = PrllExpr<ZipBuilder>::_preBuild(this->_prev1, K1{}, std::forward<H>(_h), true);
    _prev2 = PrllExpr<ZipBuilder>::_preBuild(this->_prev2, K2{},
                                             std::forward<H>(_h));
    auto obj = std::make_shared<Zip<I1, I2, K1, K2, O>>();
    obj->prev(_prev1, obj);
    obj->prev(_prev2, obj);
    DumpExpr<ZipBuilder, O>::_postBuild(obj);
    return obj;
  }

  // returns self / *this
  auto& _self() { return *this; }

  /*!
   * internally used to signal if the next unit will be the first unit in the
   * dataflow. Usually of false_type.
   * */
  std::false_type _isAddFirst;
private:
  std::shared_ptr<Source<I1>> _prev1;
  std::shared_ptr<Source<I2>> _prev2;
  H _h;
};
}
} // namespace ezl namespace ezl::detail

#endif // !ZIPBUILDER_EZL_H
