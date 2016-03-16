/*!
 * @file
 * class LoadBuilder
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef __LOADBUILDER_EZL_H__
#define __LOADBUILDER_EZL_H__

#define SUPERLA DataFlowExpr<LoadBuilder<F>>,     \
                DumpExpr<LoadBuilder<F>>,         \
                LoadExpr<LoadBuilder<F>, F>

namespace ezl {
namespace detail {

template <class T> struct DataFlowExpr;
template <class T> struct DumpExpr;
template <class T, class F> struct LoadExpr;

/*!
 * @ingroup builder
 * Builder for Load
 *
 * */
template <class F> struct LoadBuilder : public SUPERLA {
public:
  LoadBuilder(F&& sourceFunc) : LoadExpr<LoadBuilder, F>{std::forward<F>(sourceFunc)} {}

  auto& self() { return *this; }

  auto build() {
    auto obj = LoadExpr<LoadBuilder, F>::build();
    DumpExpr<LoadBuilder>::buildAdd(obj);
    return obj;
  }
};
}
} // namespace ezl namespace ezl::detail

#endif //__LOADBUILDER_EZL_H__
