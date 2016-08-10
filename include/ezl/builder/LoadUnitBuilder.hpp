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

#define LSUPER DataFlowExpr<LoadUnitBuilder<I, Fl>, Fl>

namespace ezl {
namespace detail {

template <class T, class Fl> struct DataFlowExpr;
/*!
 * @ingroup builder
 * Builder for `LoadUnit`
 * Employs crtp based design for adding expressions with
 * nearly orthogonal functionality.
 *
 * */
template <class I, class Fl> struct LoadUnitBuilder : LSUPER {
public:
  LoadUnitBuilder(std::shared_ptr<I> pr, Flow<Fl, std::nullptr_t> fl) : _prev{pr} {
    this->_fl = fl;
  }

  auto _self() { return *this; }

  auto _buildUnit() { return _prev; }

  /*!
   * returns prev unit in the dataflow
   * */
  auto prev() { return _prev; }

private:
  std::shared_ptr<I> _prev;
};
}
} // namespace ezl namespace ezl::detail

#endif // !LOADUNITBUILDER_EZL_H
