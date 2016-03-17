/*!
 * @file
 * class LoadUnitExpr
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef LOADUNITEXPR_EZL_H
#define LOADUNITEXPR_EZL_H

#include <memory>

namespace ezl {
namespace detail {

/*!
 * @ingroup builder
 * Termination expression class for `LoadUnitBuilder`
 *
 * */
template <class T, class I> struct LoadUnitExpr {

  LoadUnitExpr() = default;

  LoadUnitExpr(std::shared_ptr<I> pr) : _prev{pr} {}

  auto build() { return _prev; }

  auto prev() { return _prev; }

  auto prev(std::shared_ptr<I> p) {
    _prev = std::move(p);
    return ((T *)this)->self();
  }

  std::shared_ptr<I> _prev{nullptr};
};
}
} // namespace ezl namespace ezl::detail

#endif //!LOADUNITEXPR_EZL_H
