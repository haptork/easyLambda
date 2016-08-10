/*!
 * @file
 * class NoOp, pipeline unit for NoOping.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef NOOP_EZL_H
#define NOOP_EZL_H

#include <tuple>

#include <ezl/pipeline/Link.hpp>

namespace ezl {
namespace detail {

/*!
 * @ingroup units
 * Pipeline unit for NoOping rows based on a UDF predicate on selected columns
 * of the row.
 *
 * See examples for using with builders or unittests for direct use.
 *
 * */
template <class IO>
struct NoOp : public Link<IO, IO> {
public:
  using itype = IO;
  using otype = IO;
  static constexpr int osize = std::tuple_size<otype>::value;
  virtual void dataEvent(const itype &data) final override {
    for (auto &it : this->next()) it.second->dataEvent(data);
  }
  virtual void dataEvent(const std::vector<itype> &vData) final override {
    for (auto &it : this->next()) it.second->dataEvent(vData);
  };
};
}
} // namespace ezl ezl::detail

#endif //!NoOp_EZL_H
