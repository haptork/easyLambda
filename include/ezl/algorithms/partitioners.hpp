/*!
 * @file
 * Function objects for basic piecemean/streaming reductions
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef PARTITIONERS_EZL_ALGO_H
#define PARTITIONERS_EZL_ALGO_H

#include <tuple>

namespace ezl {

/*!
 * @ingroup algorithms
 * function object for partitioning a 1D range of given length in contiguos
 * sequences among processes
 *
 * Example usage: 
 * @code
 * .partitionBy(range(100))
 * @endcode
 *
 * */
template <class T>
class _range {
public:
  /*!
   * The possible values start from 0 and end at total - 1.
   * @param total the total number of possible values.
   */
  _range(T total) : _first{0}, _last{total - 1} {}
  /*!
   * The possible values start from first and end at last.
   * @param first the initial possible value.
   * @param first the last possible value.
   */
  _range(T first, T last) : _first{first}, _last {last} {}

  /*!
   * used for partitioning the rows while the dataflow executes
   * */  
  auto operator () (const std::tuple<const T&>& val) const {
    return (std::get<0>(val) - _first) / _share;
  }

  /*!
   * the parallel information is passed before running the dataflow
   * */
  void operator () (const int&, const std::vector<int>& procs) {
    auto len = _last - _first + 1;
    _share = T(len / procs.size());
    if (_share == 0) _share = 1;
  }

  auto reset(T total) && {
    _total = total;
    return std::move(*this);
  }
  auto& reset(T total) & {
    _total = total;
    return *this;
  }
private:
  T _total;
  T _first;
  T _last;
  T _share;
};

template <class T>
auto range(T total) {
  return _range<T>(total);
}

} // namespace ezl

#endif // !REDUCES_EZL_ALGO_H
