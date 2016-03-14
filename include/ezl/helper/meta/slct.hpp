/*!
 * @file
 * Provides custom compile time integer sequence (slct) that is used for column
 * manipulation with various operations on them such as invert mask, concatenate
 * fill etc.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef __SLCT_EZL_H__
#define __SLCT_EZL_H__

#include <type_traits>

namespace ezl {
namespace detail {
namespace meta {

/*!
 * The columns/tuple indices are from 1 to N, one of the reason being 
 * distinction between a boolean mask and integer indices. Check `saneSlct` for
 * more on boolean mask.
 * */
template <int... Ns> struct slct {
  constexpr static auto size = sizeof...(Ns);
};

// fillSlct, returns a seq with integers (L, N]; L not included N is
// requires: L<=N 
template <int L, int N, int... Ns> struct fillSlct : fillSlct<L, N - 1, N, Ns...> {};

// base case for fillSlct
template <int L, int... Ns> struct fillSlct<L, L, Ns...> { using type = slct<Ns...>; };

template <typename Slct1, typename Slct2> struct ConcatenateSlct;

template <int... T, int... U> struct ConcatenateSlct<slct<T...>, slct<U...>> {
  using type = slct<T..., U...>;
};

template <typename... Us> struct ConcatenateManySlct { using type = slct<>; };

template <typename T, typename... Us> struct ConcatenateManySlct<T, Us...> {
  using type = T;
};

template <typename T, typename U, typename... Us>
struct ConcatenateManySlct<T, U, Us...> :
    ConcatenateManySlct<typename ConcatenateSlct<T, U>::type, Us...> {};

// removes N from the sequence if exists
template <int N, int... Is>
using RemoveNslct = typename ConcatenateManySlct<
    std::conditional_t<(Is == N), slct<>, slct<Is>>...>::type;

// set difference of two seq
template <class F, class T> struct setDiff;

template <int... Fs, int T, int... Ts>
struct setDiff<slct<Fs...>, slct<T, Ts...>>: public setDiff<RemoveNslct<T, Fs...>, slct<Ts...> > {};

template <int... Fs>
struct setDiff<slct<Fs...>, slct<>> {
  using type = slct<Fs...>;
};

// inverts the slct of a fixed sequence of numbers
template <typename O, typename I> struct InvertSlctImpl;

template <int... Os, int I, int... Is>
struct InvertSlctImpl<slct<Os...>, slct<I, Is...>>
    : InvertSlctImpl<RemoveNslct<I, Os...>, slct<Is...>> {};

template <int... Os> struct InvertSlctImpl<slct<Os...>, slct<>> {
  using type = slct<Os...>;
};

template <int N, class S>
using InvertSlct = typename InvertSlctImpl<typename fillSlct<0, N>::type, S>::type;

// I size of input, O size of Output, R slct for finput This is a specilized
// function which finds the slct for a map removing the input columns and
// inserting the output columns in place of the first input column. E.g.  input
// columns to map are (int, int , int), assume column two is selected for UDF 
// which returns char. By default the output is int, int, int, char. For 
// transforming the columns are to be shuffled as 1, 4, 3 that's exactly what
// this one finds. Check tests for more.
template <int I, int O, class T> struct inPlace;

template <int I, int O, int R, int... Rs>
struct inPlace<I, O, slct<R, Rs...>> {
  using temp = typename ConcatenateManySlct<typename fillSlct<0, R>::type, 
        typename fillSlct<I, I+O>::type, typename fillSlct<R, I>::type>::type;
  using type = typename setDiff<temp, slct<R, Rs...>>::type;
};

template <int I, int O>
struct inPlace<I, O, slct<>
  > {
  using type = typename ConcatenateSlct<typename fillSlct<0, I>::type, 
        typename fillSlct<I, I+O>::type>::type;
};


// Used for checking if a slct is indeed a mask and converting to slct
// A zero or one length slct is not a mask
template <int... Is> struct isBoolImpl : public std::false_type {};

template <int I, int... Is>
struct isBoolImpl<0, I, Is...> : public isBoolImpl<I, Is...> {};

template <int I, int... Is>
struct isBoolImpl<1, I, Is...> : public isBoolImpl<I, Is...> {};

template <int... Is>
struct isBoolImpl<1, Is...> : public std::true_type {};

template <int... Is>
struct isBoolImpl<0, Is...> : public std::true_type {};

template <int... Is> struct isBool : public isBoolImpl<Is...> {};
template <> struct isBool<1> : public std::false_type {};

template <int L, int H, int... Is>
using filtered = typename ConcatenateManySlct<
    std::conditional_t<(Is < L || Is > H), slct<Is>, slct<>>...>::type;

template <class I, int... Is> struct maskToSlct;

template <int... Ns, int... Is>
struct maskToSlct<slct<Ns...>, Is...> { 
  using type = typename ConcatenateManySlct<
    std::conditional_t<(Is == 0), slct<>, slct<Ns>>...>::type;
};

template <size_t N, class S, class E = void> struct saneSlctImpl {};

template <size_t N, int... Is>
struct saneSlctImpl<N, slct<Is...>, typename std::enable_if<isBool<Is...>{}>::type> {
  static_assert(N == sizeof...(Is), "Cols selection indices suggest a boolean selection mask but\
 mask size != number of cols. Please note indices start from 1 in a non-boolean selection.");
  using type = typename maskToSlct<typename fillSlct<0, N>::type, Is...>::type;
};

template <size_t N, int... Is>
struct saneSlctImpl<N, slct<Is...>, typename std::enable_if<!isBool<Is...>{}>::type> {
  static_assert(std::is_same<filtered<1, N, Is...>, slct<>>::value, "Cols selection index out\
 of bounds. Please note indices start from 1 in a non-boolean selection.");
  using type = slct<Is...>;
};

template <size_t N, int... Ns>
using saneSlct = typename saneSlctImpl<N, slct<Ns...>>::type;

template <size_t N, class S>
using saneSlct_t = typename saneSlctImpl<N, S>::type;

}
}
} // namespace ezl detail meta

#endif //!__SEQUENCE_EZL_H__
