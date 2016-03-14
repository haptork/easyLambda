/*!
 * @file
 * class Filter, pipeline unit for filtering.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef __FILTER_EZL_H__
#define __FILTER_EZL_H__

#include <tuple>

#include "ezl/pipeline/Link.hpp"
#include "ezl/helper/meta/funcInvoke.hpp"
#include "ezl/helper/meta/slctTuple.hpp"

namespace ezl {
namespace detail {

/*!
 * @ingroup mapreduce
 * Pipeline unit for filtering rows based on a UDF predicate on selected columns
 * of the row.
 *
 * See examples for using with builders or unittests for direct use.
 *
 * */
template <class I, class Fslct, class Func, class Oslct>
struct Filter : public Link<I, typename meta::SlctTupleRefType<I, Oslct>::type> {
public:
  using itype = I;
  using otype = typename meta::SlctTupleRefType<itype, Oslct>::type;
  static constexpr int osize = std::tuple_size<otype>::value;

  Filter(Func f) : _func(f) {}

  virtual void dataEvent(const itype &data) final override {
    auto temp = meta::slctTupleRef(data, Fslct{});
    if (meta::invokeMap(_func, temp) && !this->next().empty()) {
      auto res = meta::slctTupleRef(data, Oslct{});
      for (auto &it : this->next())
        it.second->dataEvent(res);
    }
  }

private:
  Func _func;
};
}
} // namespace ezl ezl::detail

#endif //!__FILTER_EZL_H__
