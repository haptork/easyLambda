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
#ifndef __LOADUNITBUILDER_EZL_H__
#define __LOADUNITBUILDER_EZL_H__

#define LSUPER DataFlowExpr<LoadUnitBuilder<I>>,    \
               LoadUnitExpr<LoadUnitBuilder<I>, I>

namespace ezl {
namespace detail {


template <class T> struct DataFlowExpr;
template <class T, class I> struct LoadUnitExpr;
/*!
 * @ingroup builder
 * Builder for `LoadUnit`
 * Employs crtp, mixin or policy based design for adding expressions with
 * nearly orthogonal functionality. Mixin helps calling super build after
 * a template class is done with its part in building the unit.
 *
 * */
template <class I> struct LoadUnitBuilder : public LSUPER {

public:
  LoadUnitBuilder() = default;

  auto self() { return *this; }

};
}
} // namespace ezl namespace ezl::detail

#endif // !__LOADUNITBUILDER_EZL_H__
