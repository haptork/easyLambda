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
  auto operator () (const K&, const V&, const R& res) {
    //R a = 4;
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
  template<class... K, class V, class... R>
  auto operator () (const std::tuple<K...>&, const V& val, const std::tuple<R...>& res) {
    return _sum(
        val, res, std::make_index_sequence<std::tuple_size<V>::value>{});
  };


  template<class K, class V, class... Vs, class R>
  typename std::enable_if<!detail::meta::isTuple<R>{}, R>::type
  operator () (const K&, const std::tuple<const V&, const Vs&...>& val,
      const R& res) {
    return res + std::get<0>(val);
  };

private:
  template <typename V, typename R, std::size_t... index>
  inline R _sum(const V &tup1, const R &tup2,
                  std::index_sequence<index...>) {
    return std::make_tuple(std::get<index>(tup2) + (std::get<index>(tup1))...);
  }
};

} // namespace ezl

#endif // !REDUCES_EZL_ALGO_H
