/*!
 * @file
 * functions to enable selecting, shuffling and masking of columns based
 * on `slct`.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef SLCTTUPLE_EZL_H
#define SLCTTUPLE_EZL_H

#include <vector>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "ezl/helper/meta/slct.hpp"

namespace ezl {
namespace detail {
namespace meta {

template <class... T1, class... T2,
          std::size_t... i, std::size_t... j>
auto tieHelper(const std::tuple<T1...>& t1, const std::tuple<T2...>& t2,
                std::index_sequence<i...>, std::index_sequence<j...>) {
  return std::tie(std::get<i>(t1)..., std::get<j>(t2)...);
}

template <class... T1, class... T2>
auto tieTup(const std::tuple<T1...>& t1, const std::tuple<T2...>& t2) {
  return tieHelper(t1, t2, std::make_index_sequence<sizeof...(T1)>{},
                             std::make_index_sequence<sizeof...(T2)>{});
}

template <class... T1, class T2, std::size_t... i>
auto tieHelper(const std::tuple<T1...>& t1, const T2& t2,
                std::index_sequence<i...>) {
  return std::tie(std::get<i>(t1)..., t2);
}

template <class... T1, class T2>
auto tieTup(const std::tuple<T1...>& t1, const T2& t2) {
    return tieHelper(t1, t2, std::make_index_sequence<sizeof...(T1)>{});
}

template <class... T1>
const auto& tieTup(const std::tuple<T1...>& t1) {
  return t1;
}

template <class T1>
auto tieTup(const T1& t1) {
  return std::tie(t1);
}

// concatenate types of multiple tuples to single tuple type
template <typename T1, typename T2>
using TupleTieType = decltype(tieTup(std::declval<T1>(), std::declval<T2>()));

// concatenate types of multiple tuples to single tuple type
template <typename... Ts>
using TupleCatType = decltype(std::tuple_cat(std::declval<Ts>()...));

// requires slct indices within bounds
template <typename T, typename S = void> struct SlctTupleType;

template <typename Tup, int... Is>
struct SlctTupleType<Tup, slct<Is...>> {
  using type = std::tuple<
      std::decay_t<decltype(std::get<Is - 1>(std::declval<Tup>()))>...>;
};

template <typename Tup>
struct SlctTupleType<Tup, void>
    : public SlctTupleType<
          Tup, typename fillSlct<0, std::tuple_size<Tup>::value>::type> {};

template <class Tup, int... Ts>
auto slctTuple(const Tup &tup, slct<Ts...>) {
  return std::make_tuple(std::get<Ts - 1>(tup)...);
}

template <class Tup, int... Ts>
auto slctTupleRef(const Tup &tup, slct<Ts...>) {
  return std::tie(std::get<Ts - 1>(tup)...);
}

// requires slct indices within bounds
template <typename T, typename S = void> struct SlctTupleRefType;

template <typename Tup, int... Is>
struct SlctTupleRefType<Tup, slct<Is...>> {
  using type = decltype(slctTupleRef(std::declval<Tup>(), std::declval<slct<Is...>>()));
    //std::tuple<const decltype(std::get<Is - 1>(std::declval<Tup>()))&...>;
};

template <typename Tup>
struct SlctTupleRefType<Tup, void>
    : public SlctTupleRefType<
          Tup, typename fillSlct<0, std::tuple_size<Tup>::value>::type> {};

// count the columns including array elements as individual column
template<size_t N, class I>
struct colCountImpl;

template <size_t N>
struct colCountImpl<N, std::tuple<>> {
  constexpr static auto value = N;
};

template <size_t N, class I, class... Is>
struct colCountImpl<N, std::tuple<I, Is...>> : colCountImpl<N+1, std::tuple<Is...>> {};

template <size_t N, class I, size_t n, class... Is>
struct colCountImpl<N, std::tuple<std::array<I, n>, Is...>> : colCountImpl<N+n, std::tuple<Is...>> {};

template <class I>
struct colCount : colCountImpl<0, I> {};

}
}
} // namespace ezl detail meta
#endif // !SLCTTUPLE_EZL_H
