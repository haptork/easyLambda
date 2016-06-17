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

#ifndef REDUCES_EZL_ALGO_H
#define REDUCES_EZL_ALGO_H

#include <tuple>

namespace ezl {

/*!
 * @ingroup algorithms
 * function object for returning count of rows irrespective of key/value types.
 *
 * Example usage: 
 * @code
 * load<string>("*.txt").colSeparator("").rowSeparator('s')
 * .reduce<1>(ezl::count()).build()
 * @endcode
 *
 * */
class count {
public:
  template<class K, class V, class R>
  auto operator () (const R& res, const K&, const V&) {
    return 1 + res; //std::get<0>(res) + 1;
  }
};

/*!
 * @ingroup algorithms
 * function object for summation of value columns.
 *
 * Example usage: 
 * @code
 * load<string>("*.txt").colSeparator("").rowSeparator('s')
 * .reduce<1>(ezl::count()).build()
 * @endcode
 *
 * */
class sum {
public:
  template<class... R, class... K, class V>
  auto operator () (const std::tuple<R...>& res, const std::tuple<K...>&, const V& val) {
    return _sum(
        res, val, std::make_index_sequence<std::tuple_size<V>::value>{});
  };

  template<class K, class V, class... Vs, class R>
  typename std::enable_if<!detail::meta::isTuple<R>{}, R>::type
  operator () (const R& res, const K&, const std::tuple<const V&, 
      const Vs&...>& val) {
    return res + std::get<0>(val);
  };

private:
  template <typename R, typename V, std::size_t... index>
  inline R _sum(const R &tup1, const V &tup2,
                  std::index_sequence<index...>) {
    return std::make_tuple(std::get<index>(tup1) + (std::get<index>(tup2))...);
  }
};

} // namespace ezl

#endif // !REDUCES_EZL_ALGO_H
