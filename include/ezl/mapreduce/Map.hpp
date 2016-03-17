/*!
 * @file
 * class Map.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef MAP_EZL_H
#define MAP_EZL_H

#include <tuple>
#include <type_traits>

#include "ezl/pipeline/Link.hpp"
#include "ezl/helper/meta/funcInvoke.hpp"
#include "ezl/helper/meta/slctTuple.hpp"
#include "ezl/helper/meta/typeInfo.hpp"

namespace ezl {
namespace detail {

/*!
 * @ingroup mapreduce
 * Map unit for transforming a row into zero, one or many new rows.
 * The UDF can be a tuple or free params for columns. The columns
 * to pass to UDF can be selected. The output from the unit can be
 * selected from input and output columns.
 *
 * UDF may return a single value for returning a single column or a tuple
 * for multiple columns. 
 * A vector may be returned for returning variable number of rows for each
 * input row.
 *
 * See examples for using with builders or unittests for direct use.
 *
 * */
template <class Types>
struct Map : public Link<typename Types::I, typename Types::otype> {
public:
  using itype = typename Types::I;
  using otype = typename Types::otype;
  using Func = typename Types::F;
  using Fslct= typename Types::S;
  using Oslct= typename Types::O;
  static constexpr int isize = std::tuple_size<itype>::value;
  static constexpr int osize = std::tuple_size<otype>::value;

  Map(Func func) : _func(func) {}

  virtual void dataEvent(const itype &data) final override {
    callEm<decltype(meta::invokeMap(_func, 
      meta::slctTupleRef(data, Fslct{})))>(data);
  }

  template <class T>
  auto callEm(const itype &data, typename std::enable_if<meta::isVector<T>{}>::type* dummy = 0) {
    auto temp = meta::slctTupleRef(data, Fslct());
    auto funOut = meta::invokeMap(_func, temp);
    if (Link<itype, otype>::next().empty()) return;
    std::vector<otype> res;
    res.reserve(funOut.size());
    for (const auto& it : funOut) {
      res.emplace_back(meta::slctTupleRef(meta::tieTup(data, it), Oslct{}));
     }
     for (auto &it : Link<itype, otype>::next()) {
       it.second->dataEvent(res);
     }
   }

  template <class T>
  auto callEm(const itype &data, typename std::enable_if<!meta::isVector<T>{}>::type* dummy = 0) {
    auto temp = meta::slctTupleRef(data, Fslct());
    auto result = meta::invokeMap(_func, temp);
    if (Link<itype, otype>::next().empty()) return;
    auto otemp = meta::slctTupleRef(meta::tieTup(data, result), Oslct{});
    for (auto &it : Link<itype, otype>::next()) {
      it.second->dataEvent(otemp);
    }
  }
  // TODO: skip cat and slct if otemp == mtemp, after benchmarking
    
private:
  Func _func;
};
}
} // namespace ezl ezl::detail

#endif // ! MAP_EZL_H
