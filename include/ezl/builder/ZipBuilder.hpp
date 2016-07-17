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

#include <ezl/mapreduce/Zip.hpp>


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
 * Employs crtp
 *
 * */
template <class I1, class I2, class K1, class K2, class O, class H, class A>
struct ZipBuilder : ZSUPER {
public:
  ZipBuilder(std::shared_ptr<Source<I1>> prev1,
             std::shared_ptr<Source<I2>> prev2, std::shared_ptr<A> a)
      : _prev1{prev1}, _prev2{prev2} {
    this->prll(Karta::prllRatio);
    this->_fl = a;
  }

  auto& self() { return *this; }

  template <class NO>
  auto colsSlct(NO = NO{}) {  // NOTE: while calling with T cast arg. is req
    auto temp = ZipBuilder<I1, I2, K1, K2, NO, H, A>{std::move(this->_prev1),
                                                  std::move(this->_prev2), std::move(this->_fl)};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  template <class NH>
  auto hash() {
    auto temp = ZipBuilder<I1, I2, K1, K2, O, NH, A>{std::move(this->_prev1), std::move(this->_prev2), std::move(this->_fl)};
    temp.props(this->props());
    temp.dumpProps(this->dumpProps());
    return temp;
  }

  auto buildUnit() {
    _prev1 = PrllExpr<ZipBuilder>::preBuild(this->_prev1, K1{}, H{});
    _prev2 = PrllExpr<ZipBuilder>::preBuild(this->_prev2, K2{}, H{});
    auto obj = std::make_shared<Zip<I1, I2, K1, K2, O, H>>();
    obj->prev(_prev1, obj);
    obj->prev(_prev2, obj);
    DumpExpr<ZipBuilder, O>::postBuild(obj);
    return obj;
  }

  std::false_type isAddFirst;
private:
  std::shared_ptr<Source<I1>> _prev1;
  std::shared_ptr<Source<I2>> _prev2;
};
}
} // namespace ezl namespace ezl::detail

#endif // !ZIPBUILDER_EZL_H
