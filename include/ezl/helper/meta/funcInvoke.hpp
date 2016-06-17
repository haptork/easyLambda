/*!
 * @file
 * functions to invoke a function with params as tuple expanded or unexpanded
 * for map and reduce with uniform interface. For reduceAll vector of tuple or
 * tuple of vectors expanded or unexpanded.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef FUNCINVOKE_EZL_H
#define FUNCINVOKE_EZL_H

#include <vector>
#include <tuple>
#include <type_traits>

#include <ezl/helper/meta/slct.hpp>

namespace ezl {
namespace detail {
namespace meta {

// to test if a function type can be called with the given argument types
// @source
// http://stackoverflow.com/questions/22882170/c-compile-time-predicate-to-test-if-a-callable-object-of-type-f-can-be-called
struct can_call_test {
  template <typename F, typename... A>
  static decltype(std::declval<F>()(std::declval<A&>()...), std::true_type())
  f(int);

  template <typename F, typename... A> static std::false_type f(...);
};

template <typename F, typename... A>
struct can_call : decltype(can_call_test::f<F, A ...>(0)) {};
/*
template <typename F, typename... A>
struct can_call<F(A...)> : can_call<F, A ...> {};
*/
// invoke if callable else ignore, returning a variant would be a better option.
template <class F, class... As>
typename std::enable_if<can_call<F &&, As &&...>{}, bool>::type
invokeFallBack(F &&f, As &&... args) {
  f(std::forward<As>(args)...);
  return true;
}

// invoke if callable else ignore, returning a variant would be a better option.
template <class F, class... As>
typename std::enable_if<!can_call<F &&, As &&...>{}, bool>::type
invokeFallBack(F &&f, As &&... args) {
  return false;
}

template <class R, class F, class... As>
typename std::enable_if<can_call<F &&, As &&...>{}, bool>::type
invokeResFallBack(R &res, F &&f, As&&... args) {
  res = f(std::forward<As>(args)...);
  return true;
}

template <class R, class F, class... As>
typename std::enable_if<!can_call<F &&, As &&...>{}, bool>::type
invokeResFallBack(R &res, F &&f, As &&... args) {
  return false;
}

// function callers for different set of tuples and vector of tuples required
// for map and reduce
template <typename Func, typename Tup, std::size_t... index, typename... Args>
decltype(auto) invokeHelperOneTup(Func &&func, Tup &&tup,
                             std::index_sequence<index...>, Args&&... args) {
  return func(std::get<index>(std::forward<Tup>(tup))..., std::forward<Args>(args)...);
}

template <typename Func, typename Tup, std::size_t... index, typename Tup2,
          std::size_t... index2, typename... Args>
decltype(auto) invokeHelperTwoTup(Func &&func, Tup &&tup,
                             std::index_sequence<index...>, Tup2 &&tup2,
                             std::index_sequence<index2...>, Args&&... args) {
  return func(std::forward<Args>(args)..., std::get<index>(std::forward<Tup>(tup))...,
              std::get<index2>(std::forward<Tup2>(tup2))...);
}

template <typename Func, typename Tup, std::size_t... index, typename Tup2,
          std::size_t... index2, typename Tup3, std::size_t... index3>
inline decltype(auto) invokeHelperThreeTup(Func &&func, Tup &&tup,
                             std::index_sequence<index...>, Tup2 &&tup2,
                             std::index_sequence<index2...>, Tup3 &&tup3,
                             std::index_sequence<index3...>) {
  return func(std::get<index>(std::forward<Tup>(tup))...,
              std::get<index2>(std::forward<Tup2>(tup2))...,
              std::get<index3>(std::forward<Tup3>(tup3))...);
}

// map function calls tuple of arguments
// sizeof is necessary because a function with param having a tuple with
// single element can be called expanded by compiler itself which creates
// ambigous call i.e. both expanded and unexpanded can_call return true.
template <class F, class... As>
inline decltype(auto) invokeMap(
    F &&func, const std::tuple<As...> &args,
    typename std::enable_if</* !can_call<F, As...>{} &&*/
    can_call<F, decltype(args)>{}>::type *dummy = 0) {
  return func(args);
}

// map function calls arguments separetely
template <class F, class... As>
inline decltype(auto) invokeMap(F &&func, const std::tuple<As...> &args,
            typename std::enable_if<!can_call<F, decltype(args)>{}
            /*can_call<F, As...>{}*/>::type *dummy = 0) {
  static_assert(
      (can_call<F, As...>{}),
      "Map fn. can not be called with columns selected as parameters.");
  return invokeHelperOneTup(std::forward<F>(func), args,
                       std::make_index_sequence<sizeof...(As)>{});
}

// reduce-all AOS calls key as separate arguments
template <class F, class... Ks, class... Vs>
inline decltype(auto) invokeReduceAll(F &&func, const std::tuple<Ks...> &key,
            const std::vector<std::tuple<Vs...>> &val,
            typename std::enable_if<can_call<F, Ks..., decltype(val)>{}
            && !can_call<F, decltype(key), decltype(val)>{}
            >::type *
                dummy = 0) {
  return invokeHelperOneTup(std::forward<F>(func), key,
                       std::make_index_sequence<sizeof...(Ks)>{}, val);
}

// reduce-all AOS with key as tuple
// sizeof is necessary because a function with param having a tuple with
// single element can be called expanded by compiler itself which creates
// ambigous call i.e. both expanded and unexpanded can_call return true.
template <class F, class... Ks, class... Vs>
inline decltype(auto) invokeReduceAll(F &&f, const std::tuple<Ks...> &key,
            const std::vector<std::tuple<Vs...>> &val,
            typename std::enable_if< 
                can_call<F, decltype(key), decltype(val)>{}>::type *dummy = 0) {
  return f(key, val);
}

// reduce-all AOS not well formed
template <class F, class... Ks, class... Vs>
auto invokeReduceAll(
    F &&func, const std::tuple<Ks...> &key,
    const std::vector<std::tuple<Vs...>> &val,
    typename std::enable_if<
        !can_call<F, Ks..., decltype(val)>{} &&
        !can_call<F, std::tuple<Ks...>, decltype(val)>{}>::type *dummy = 0) {
  static_assert(
      true,
      "reduceAll fn. can not be called with columns selected as parameters.");
}

// reduce-all SOA with key as well as value as tuple
template <class F, class... Ks, class... Vs>
inline decltype(auto) invokeReduceAll(F &&f, const std::tuple<Ks...> &key, const std::tuple<Vs...> &val,
            typename std::enable_if<
                can_call<F &&, decltype(key), decltype(val)>{}>::type *dummy = 0) {
  return f(key, val);
}

// reduce-all SOA with key as well as value as separate arguments
template <class F, class... Ks, class... Vs>
inline decltype(auto) invokeReduceAll(F &&f, const std::tuple<Ks...> &tup1, const std::tuple<Vs...> &tup2,
            typename std::enable_if<can_call<F &&, Ks..., Vs...>{}>::type *d = 0
            ) {
  return invokeHelperTwoTup(std::forward<F>(f), tup1,
                       std::make_index_sequence<sizeof...(Ks)>{}, tup2,
                       std::make_index_sequence<sizeof...(Vs)>{});
}

// reduce-all SOA function is not well formed
template <class F, class... Ks, class... Vs>
inline decltype(auto) invokeReduceAll(F &&f, const std::tuple<Ks...> &, const std::tuple<Vs...> &,
            typename std::enable_if<
                !can_call<F &&, Ks..., Vs...>{} &&
                !can_call<F &&, std::tuple<Ks...>, std::tuple<Vs...>>{}>::type *
                dummy = 0) {
  static_assert(
      true,
      "reduceAll fn. can not be called with columns selected as parameters.");
  return;
}

// reduce as tuple
// sizeof is necessary because a function with param having a tuple with
// single element can be called expanded by compiler itself which creates
// ambigous call i.e. both expanded and unexpanded can_call return true.
template <class F, class O, class... Ks, class... Vs>
inline decltype(auto) invokeReduce(
    F &&f, O& o, const std::tuple<Ks...>& k, const std::tuple<Vs...>& v,
    typename std::enable_if<can_call<F &&, O&, std::tuple<Ks...>, 
                                       std::tuple<Vs...>>{}>::type *dummy = 0) {
  return f(o, k, v);
}

// reduce with all expanded
template <class F, class... Os, class... Ks, class... Vs>
inline decltype(auto) invokeReduce(F &&f, std::tuple<Os&&...>& out, 
            const std::tuple<Ks&&...>& key, const std::tuple<Vs&&...>& val,
            typename std::enable_if<
                !can_call<F &&, std::tuple<Os&&...>, std::tuple<Ks&&...>,
                                     std::tuple<Vs&&...>>{} &&
                !can_call<F &&, Os&&..., Ks&&..., std::tuple<Vs&&...>>{} &&
                can_call<F &&, Os&&..., Ks&&..., Vs&&...>{}>::type *dummy = 0) {
  return invokeHelperThreeTup(std::forward<F>(f), out,
                       std::make_index_sequence<sizeof...(Os)>{}, key,
                       std::make_index_sequence<sizeof...(Ks)>{}, val,
                       std::make_index_sequence<sizeof...(Vs)>{});
}

// reduce with all expanded, output result as is
template <class F, class O, class... Ks, class... Vs>
inline decltype(auto) invokeReduce(F &&f, O& o, const std::tuple<Ks...>& key,
            const std::tuple<Vs...>& val,
            typename std::enable_if< 
                !can_call<F &&, O&, std::tuple<Ks...>, std::tuple<Vs...>>{} &&
                can_call<F &&, O&, Ks..., Vs...>{}>::type *dummy = 0) {
  return invokeHelperTwoTup(std::forward<F>(f), key,
                       std::make_index_sequence<sizeof...(Ks)>{}, val,
                       std::make_index_sequence<sizeof...(Vs)>{}, o);
}

// reduce is not well formed
template <class F, class O, class... Ks, class... Vs>
inline decltype(auto) invokeReduce(F &&reduceUDF, O& res, 
            const std::tuple<Ks...>& key, const std::tuple<Vs...>& val,
            typename std::enable_if<
                !can_call<F &&, O&, std::tuple<Ks...>, std::tuple<Vs...>
                          >{} && !can_call<F &&, O&, Ks..., Vs...>{}>::type *dummy = 0) {
  return reduceUDF(res, key, val); // reduceUDF is ill formed.
}

// reduce is not well formed
template <class F, class... Os, class... Ks, class... Vs>
inline decltype(auto) invokeReduce(F &&reduceUDF, const std::tuple<Os...>& res, const std::tuple<Ks...>& key, const std::tuple<Vs...>& val,
            typename std::enable_if<
                !can_call<F &&, std::tuple<Os...>, std::tuple<Ks...>, 
                          std::tuple<Vs...>>{} &&
                !can_call<F &&, Os..., Ks..., Vs...>{}>::type *dummy = 0) {
  return reduceUDF(res, key, val); // reduceUDF is ill formed.
}

}
}
} // namespace ezl detail meta
#endif //!FUNCINVOKE_EZL_H
