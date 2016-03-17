/*!
 * @file
 * class Load, root unit for loading data.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef LOAD_EZL_H
#define LOAD_EZL_H

#include <vector>
#include <functional>

#include "ezl/pipeline/Root.hpp"
#include "ezl/helper/meta/typeInfo.hpp"

namespace ezl {
namespace detail {

/*!
 * @ingroup mapreduce
 * A root unit in a pipeline, which produces data using a UDF.
 *
 * The UDF may return a single value for a column or tuple for multiple
 * columns. A vector may be returned in case of returning variable
 * number of rows for each input row.
 *
 * For single row at a time, a pair/tuple of (row, 'is end of data) is to be
 * returned. For a vector return type, an empty vector represents 'end of data'.
 *
 * A variant once introduced in standard will be better suited than the pair
 * with a flag. :(
 *
 * A UDF can optionally be called with (int, std::vector<int>), in which case
 * the ranks on which the unit is running and index of current process rank in
 * the list is passed to the UDF just before the data flow starts. This can be
 * used to produce different data based on rank.
 *
 * E.g. UDFs:
 * [](){ return make_pair{3, true}; }; 
 * [](){ return vector<int>{}; }; 
 * [](){ return vector<tuple<int, float>>{}; }; 
 *
 * See `kick`, `LoadMem`... examples in `io.hpp` for generic UDFs.
 *
 * */
template <class F, class I = typename meta::LoadTypes<F>::type> 
class Load: public Root<I> {
    
public:
  using otype = I;
  static constexpr int osize = std::tuple_size<I>::value;

  Load(ProcReq r, F&& f, std::pair<int, std::vector<int>>* procSink)
      : Root<otype>{r}, _func{std::forward<F>(f)}, _procSink{procSink} {}

private:
  virtual void _pullData() override final {
    if (_procSink) {
      _procSink->first = this->par().pos();
      _procSink->second = this->par().procAll();
    }
    meta::invokeFallBack(_func, this->par().pos(), this->par().procAll());
    while(callEm<decltype(_func())>()) {}
  }

  template <class T>
  auto callEm(typename std::enable_if<!meta::isVector<T>{}>::type*
      dummy = 0) {
    decltype(auto) res = _func();
    if (!std::get<1>(res)) return false;
    auto rowRef = meta::tieTup(std::get<0>(res));
    for (auto &it : this->next()) {
      it.second->dataEvent(rowRef);
    }
    return true;
  }

  template <class T>
  auto callEm(typename std::enable_if<meta::isVector<T>{}>::type*
      dummy = 0) {
    decltype(auto) rows = _func();
    if (rows.empty()) {
      return false;
    }
    for (auto &row : rows) {
      for (auto &it : this->next()) {
        it.second->dataEvent(meta::tieTup(row));
      }
    }
    return true;
  }

  F _func;
  bool _share {false};
  std::pair<int, std::vector<int>>* _procSink;
};

}
} // namespace ezl ezl::detail

#endif // !LOAD_EZL_H
