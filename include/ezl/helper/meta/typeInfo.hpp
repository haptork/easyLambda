/*!
 * @file
 * Type info related to general operations, Map and Reduce(All) classes.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef TYPEINFO_EZL_H
#define TYPEINFO_EZL_H

#include <vector>
#include <tuple>
#include <type_traits>

#include <boost/functional/hash.hpp>

#include <ezl/helper/meta/funcInvoke.hpp>
#include <ezl/helper/meta/slctTuple.hpp>
#include <ezl/helper/meta/slct.hpp>

namespace ezl { namespace detail {
namespace meta {

// gives a uniform way/data-type wrapper abstraction by converting a type, tuple
// of types, vector of type or vector of tuple in the tuple and vector of tuple
// format. user defined function (UDF) can return in any of the format and it
// can be fetched in the same way as tuple or vector of tuple.
//
// Not the best way but works for now.
// TODO: implement in a way to reduce redundant code @haptork

// For free types, delegates to tuple type
/*
template <typename... Ts>
struct GetTupleType { // TODO: 1-decay 2-AOS2SOA @haptork
  using type = std::tuple<Ts...>;
  static auto getVTResults(Ts&&... t) {
    return GetTupleType<type>::getVTResults(std::make_tuple(std::forward<Ts>(t)...));
  }
  static auto getTResults(Ts... t) {
    return GetTupleType<type>::getTResults(std::make_tuple(std::forward<Ts>(t)...));
  }
};
*/ 
template <typename T>
struct GetTupleType { // TODO: 1-decay 2-AOS2SOA @haptork
  using type = std::tuple<T>;
  auto getVTResults(T&& t) {
    std::vector<T> v;
    v.push_back(std::forward<T>(t));
    return v;
  }
  auto getTResults(T t) {
    return t;
  }
};


template <typename... Ts> struct GetTupleType<std::tuple<Ts...>> {
  using type = std::tuple<Ts...>;
  auto getVTResults(std::tuple<Ts...>&& a) {
    std::vector<type> v;
    v.push_back(std::move(a));
    return v;
  }
  auto getTResults(std::tuple<Ts...> a) { return a; }
};

template <typename T> struct GetTupleType<std::vector<T>> {
  using type = std::tuple<typename std::vector<T>::value_type>;
  auto getVTResults(std::vector<T> a) {
    return a;
  }
};

template <typename... Ts> struct GetTupleType<std::vector<std::tuple<Ts...>>> {
  using type = std::tuple<Ts...>;
  auto getVTResults(std::vector<std::tuple<Ts...>> a) { return a; }
};

// a collection of types that might be needed by a map based on the
// input data type, selection and function.
template <class I, class MSlct, class Func> struct MapDefTypes {
  using finp = typename SlctTupleType<I, MSlct>::type;
  using fout = typename GetTupleType<decltype(invokeMap(
      std::declval<typename std::add_lvalue_reference<Func>::type>(),
      std::declval<typename std::add_lvalue_reference<finp>::type>()))>::
      type;
  using odefslct = typename fillSlct<0, std::tuple_size<I>::value +
                                      std::tuple_size<fout>::value>::type;
};

// a collection of types that might be needed by a map based on the
// input data type, selection and function.
template <class I, class Func> struct MapDefTypesNoSlct {
  using fout = typename GetTupleType<decltype(invokeMap(
      std::declval<typename std::add_lvalue_reference<Func>::type>(),
      std::declval<typename std::add_lvalue_reference<I>::type>()))>::
      type;
  using odefslct = typename fillSlct<0, std::tuple_size<I>::value +
                                      std::tuple_size<fout>::value>::type;
};

// a collection of types that might be needed by a map based on the
// input data type, selection, function and output type.
template <class IT, class MSlct, class Func, class OSlct> struct MapTypes {
  using I = IT;
  using S = MSlct;
  using F = Func;
  using finp = typename SlctTupleRefType<I, MSlct>::type;
  using fout = typename GetTupleType<decltype(invokeMap(
      std::declval<typename std::add_lvalue_reference<Func>::type>(),
      std::declval<typename std::add_lvalue_reference<finp>::type>()))>::
      type;
  constexpr static auto outsize =
      std::tuple_size<I>::value + std::tuple_size<fout>::value;
  using O = saneSlct_t<outsize, OSlct>;
  using otype = typename SlctTupleRefType<TupleTieType<I, fout>, O>::type;
};

// BufType is needed for reduce-all in addition to tuple type as a
// type wrapper extraction for the SOA/AOS type used in UDF.
//
// Again, not the best way but works for now.
template <typename F, typename KT, typename VT, typename enable = void>
struct BufType { 
  static_assert(!std::is_same<enable, void>::value, "Function provided for\
 reduceAll does not match the columns.");
};

// buffer type for SOA
template <typename F, typename... Ks, typename... Vs>
struct BufType<
    F, std::tuple<Ks...>, std::tuple<Vs...>,
    typename std::enable_if<(
        can_call<F, std::tuple<Ks...>, std::tuple<std::vector<Vs>...>>{} ||
        can_call<F, Ks..., std::vector<Vs>...>{})>::type> {
  using type = std::tuple<std::vector<Vs>...>;
};

// buffer type for AOS
template <typename F, typename... Ks, typename... Vs>
struct BufType<
    F, std::tuple<Ks...>, std::tuple<Vs...>,
    typename std::enable_if<(
        can_call<F, std::tuple<Ks...>, std::vector<std::tuple<Vs...>>>{} ||
        can_call<F, Ks..., std::vector<std::tuple<Vs...>>>{})>::type> {
  using type = std::vector<std::tuple<Vs...>>;
};

// a collection of types for getting type info for reduce-all with default
// output selection input data type, selection and function.
template <class I, class Kslct, class Fslct, class Func> 
struct ReduceAllDefTypes {
  using ktype = typename SlctTupleRefType<I, Kslct>::type;
  using vtype = typename SlctTupleType<I, Fslct>::type;
  using buftype = typename BufType<Func, ktype, vtype>::type;
  using fout = typename GetTupleType<decltype(invokeReduceAll(
      std::declval<typename std::add_lvalue_reference<Func>::type>(),
      std::declval<typename std::add_lvalue_reference<ktype>::type>(),
      std::declval<typename std::add_lvalue_reference<buftype>::type>()))>::
      type;
  using odefslct = typename fillSlct<0, std::tuple_size<ktype>::value +
                                       std::tuple_size<fout>::value>::type;
};

// a collection of types for getting type info for reduce-all with default
// output selection input data type, selection, function and output selection.
template <class I, class K, class V, class F, class O>
struct ReduceAllTypes {
  using itype = I;
  using Kslct = K;
  using Vslct = V;
  using Func = F;
  using ktype = typename SlctTupleType<I, K>::type;
  using kreftype = typename SlctTupleRefType<I, K>::type;
  using vtype = typename SlctTupleType<I, V>::type;
  using buftype = typename BufType<F, ktype, vtype>::type;
  using fout = typename GetTupleType<decltype(invokeReduceAll(
      std::declval<typename std::add_lvalue_reference<F>::type>(),
      std::declval<typename std::add_lvalue_reference<ktype>::type>(),
      std::declval<typename std::add_lvalue_reference<buftype>::type>()))>::
      type;
  constexpr static auto outsize =
      std::tuple_size<ktype>::value + std::tuple_size<fout>::value;
  using Oslct = saneSlct_t<outsize, O>;
  using otype =
      typename SlctTupleRefType<TupleTieType<ktype, fout>, 
                                          saneSlct_t<outsize, Oslct>>::type;
};

// a collection of types for getting type info for reduce with default
// output selection input data type, selection and function.
template <class I, class Kslct, class fout>
struct ReduceDefTypes {
  using ktype = typename SlctTupleRefType<I, Kslct>::type;
  //using vtype = typename SlctTupleRefType<I, Vslct>::type;
  using odefslct = typename fillSlct<0, std::tuple_size<ktype>::value +
                                       std::tuple_size<fout>::value>::type;
};

// a collection of types for getting type info for reduce with default
// output selection input data type, selection, function and output selection.
template <class I, class Kslct, class Vslct, class F, class fout, class OSlct>
struct ReduceTypes {
  using itype = I;
  using K = Kslct;
  using V = Vslct;
  using Func = F;
  using FO = fout;
  using ktype = typename SlctTupleType<I, Kslct>::type;
  using kreftype = typename SlctTupleRefType<I, Kslct>::type;
  using ftuple = typename GetTupleType<fout>::type;
  constexpr static auto outsize =
      std::tuple_size<ktype>::value + std::tuple_size<ftuple>::value;
  using Oslct = saneSlct_t<outsize, OSlct>;
  using otype =
      typename SlctTupleRefType<TupleTieType<ktype, ftuple>, 
                                          saneSlct_t<outsize, OSlct>>::type;
  using vtype = typename SlctTupleRefType<I, Vslct>::type;
  constexpr static bool isRefRes = std::is_lvalue_reference<decltype(invokeReduce(
          std::declval<typename std::add_lvalue_reference<F>::type>(),
          std::declval<typename std::add_lvalue_reference<fout>::type>(),
          std::declval<typename std::add_lvalue_reference<ktype>::type>(),
          std::declval<typename std::add_lvalue_reference<vtype>::type>()
          ))>::value;
};

template <class F, class P> struct RiseTypesImpl {
  using type = typename meta::SlctTupleRefType<
                  typename GetTupleType<
                    std::decay_t<decltype((std::get<0>(std::declval<P>())))>
                  >::type
               >::type;
};

template <class F, class T, class A> struct RiseTypesImpl<F, std::vector<T, A>> {
  using type = typename meta::SlctTupleRefType<typename GetTupleType<T>::type>::type;
};

// types for Rise
template <class F> 
struct RiseTypes : RiseTypesImpl<F, decltype(std::declval<F>()())> {};

template <class I, int... Ns>
using HashType = boost::hash<
    typename meta::SlctTupleRefType<I, meta::saneSlct<std::tuple_size<I>::value, Ns...>>::type>;
}
}} // namespace ezl detail meta
#endif //!TYPEINFO_EZL_H
