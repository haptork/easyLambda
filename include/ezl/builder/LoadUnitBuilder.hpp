/*!
 * @file
 * class LoadUnitBuilder
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef LOADUNITBUILDER_EZL_H
#define LOADUNITBUILDER_EZL_H

#define LSUPER DataFlowExpr<LoadUnitBuilder<I, O, A>, A>

namespace ezl {
namespace detail {

template <class T, class A> struct DataFlowExpr;
/*!
 * @ingroup builder
 * Builder for `LoadUnit`
 * Employs crtp based design for adding expressions with
 * nearly orthogonal functionality.
 *
 * */
template <class I, class O, class A> struct LoadUnitBuilder : LSUPER {
public:
  LoadUnitBuilder(std::shared_ptr<I> pr) : _prev{pr} {
    this->_fl = std::make_shared<A>();
  }

  LoadUnitBuilder(std::shared_ptr<I> pr, std::shared_ptr<A> a) : _prev{pr} {
    this->_fl = a;
  }

  auto self() { return *this; }

  auto buildUnit() { return _prev; }

  auto prev() { return _prev; }

  O isAddFirst;
private:
  std::shared_ptr<I> _prev;
};
}
} // namespace ezl namespace ezl::detail

#endif // !LOADUNITBUILDER_EZL_H
