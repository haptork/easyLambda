/*!
 * @file
 * overloaded functions to provide same interface for push, clear... in 
 * tuple<vector, vec..> (SOA) and vector<tuple<...>> (AOS)
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef __COHERENTVECTOR_EZL_H__
#define __COHERENTVECTOR_EZL_H__

#include <vector>
#include <tuple>
#include <type_traits>

#include "ezl/helper/meta/slct.hpp"

namespace ezl {
namespace detail {
namespace meta {

template <int N, typename... Is>
void coherentPushImpl(std::tuple<std::vector<Is>...> &v,
                      const std::tuple<Is...> &t,
                      typename std::enable_if<N == 0>::type *dummy = 0) {
  std::get<N>(v).emplace_back(std::get<N>(t));
}

template <int N, typename... Is>
void coherentPushImpl(std::tuple<std::vector<Is>...> &v,
                      const std::tuple<Is...> &t,
                      typename std::enable_if<N != 0>::type *dummy = 0) {
  std::get<N>(v).emplace_back(std::get<N>(t));
  coherentPushImpl<N - 1, Is...>(v, t);
}

template <typename... Is>
auto coherentPush(std::tuple<std::vector<Is>...> &v,
                  const std::tuple<Is...> &t) {
  coherentPushImpl<sizeof...(Is)-1, Is...>(v, t);
  return int(std::get<0>(v).size());
}

template <typename... Is>
auto coherentPush(std::vector<std::tuple<Is...>> &v,
                  const std::tuple<Is...> &t) {
  v.emplace_back(t);
  return int(v.size());
}

template <int N, typename... Is>
void coherentPopFrontImpl(std::tuple<std::vector<Is>...> &v,
                          typename std::enable_if<N == 0>::type *dummy = 0) {
  std::get<N>(v).erase(std::begin(std::get<N>(v)));
}

template <int N, typename... Is>
void coherentPopFrontImpl(std::tuple<std::vector<Is>...> &v,
                          typename std::enable_if<N != 0>::type *dummy = 0) {
  std::get<N>(v).erase(std::begin(std::get<N>(v)));
  coherentPopFrontImpl<N - 1, Is...>(v);
}

template <typename... Is>
auto coherentPopFront(std::tuple<std::vector<Is>...> &v) {
  assert(!(std::get<0>(v).empty())); // assuming all are equal size.
  coherentPopFrontImpl<sizeof...(Is)-1, Is...>(v);
  return int(std::get<0>(v).size());
}

template <typename... Is>
auto coherentPopFront(std::vector<std::tuple<Is...>> &v) {
  assert(!v.empty());
  v.erase(std::begin(v));
  return int(v.size());
}

template <int N, typename... Is>
void coherentClearImpl(std::tuple<std::vector<Is>...> &v,
                          typename std::enable_if<N == 0>::type *dummy = 0) {
  std::get<N>(v).clear();
}

template <int N, typename... Is>
void coherentClearImpl(std::tuple<std::vector<Is>...> &v,
                          typename std::enable_if<N != 0>::type *dummy = 0) {
  std::get<N>(v).clear();
  coherentClearImpl<N - 1, Is...>(v);
}

template <typename... Is>
auto coherentClear(std::tuple<std::vector<Is>...> &v) {
  assert(!(std::get<0>(v).empty())); // assuming all are equal size.
  coherentClearImpl<sizeof...(Is)-1, Is...>(v);
}

template <typename... Is>
auto coherentClear(std::vector<std::tuple<Is...>> &v) {
  assert(!v.empty());
  v.clear();
}

template <typename... Is>
auto coherentSize(const std::tuple<std::vector<Is>...> &v) {
  return int(std::get<0>(v).size()); // assuming all are equal size.
}

template <typename... Is>
auto coherentSize(const std::vector<std::tuple<Is...>> &v) {
  return int(v.size());
}
}
}
} // namespace ezl detail meta

#endif // !__COHERENTVECTOR_EZL_H__
