/*!
 * @file
 * Function objects for basic filtering
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef __FILTERS_EZL_ALGO_H__
#define __FILTERS_EZL_ALGO_H__

#include <tuple>

namespace ezl {
namespace detail {

/*!
 * @ingroup algorithms
 * callable for equals to a reference number.
 *
 * Can be used to filter rows based on equality with a refence.
 *
 * E.g.: 
 * load<int, char, float>(filename)
 * .filter<2>(eq('c'))
 *
 * The above example filters all the rows that have 2nd column as 'c'.
 * */
template <class Ref>
class _eq {
public:
  _eq(Ref r) { _ref = r; }

  template <class T>
  bool operator () (const T& row) {
    return row == _ref;
  }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * callable for equals to a reference number and an index in the array
 *
 * Can be used to filter rows based on equality with a refence.
 * E.g.:
 * load<array<int, 3>>(filename)
 * .filter(eqAr<2>(0))
 *
 * The above example filters all the rows where second element of the
 * array is equal to 0.
 *
 * Can be used with any types that support std::get such as pair, array...
 *
 * */
template <size_t N, class Ref>
class _eqAr{
public:
  _eqAr(Ref r) { _ref = r; }

  template <class T>
  bool operator () (const T& row) {
    return std::get<N>(std::get<0>(row)) == _ref;
  }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * callable for greater than a reference number
 * 
 * Usage is same as `eq`
 * */
template <class Ref>
class _gt {
public:
  _gt(Ref r) { _ref = r; }

  template <class T>
  bool operator () (const T& row) {
    return row > _ref;
  }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * callable for greater than a reference number and an index in the array
 *
 * Usage is same as `eqAr`.
 * */ 
template <size_t N, class Ref>
class _gtAr{
public:
  _gtAr(Ref r) { _ref = r; }

  template <class T>
  bool operator () (const T& row) {
    return std::get<N>(std::get<0>(row)) > _ref;
  }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * callable for less than a reference number
 * 
 * Usage is same as `eq`
 * */
template <class Ref>
class _lt {
public:
  _lt(Ref r) { _ref = r; }

  template <class T>
  bool operator () (const T& row) {
    return row < _ref;
  }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * callable for less than a reference number and an index in the array
 *
 * Usage is same as `eqAr`.
 * */ 
template <size_t N, class Ref>
class _ltAr{
public:
  _ltAr(Ref r) { _ref = r; }

  template <class T>
  bool operator () (const T& row) {
    return std::get<N>(std::get<0>(row)) < _ref;
  }

private:
  Ref _ref;
};


} // namespace ezl::detail

// functions for all above filters
template <class... Ts>
inline auto eq(Ts... ts) {
  return detail::_eq<std::tuple<Ts...>>{std::make_tuple(ts...)};
}

template <size_t N, class T>
inline auto eqAr(T t) {
  return detail::_eqAr<N-1, T>{t};
}

template <class... Ts>
inline auto gt(Ts... ts) {
  return detail::_gt<std::tuple<Ts...>>{std::make_tuple(ts...)};
}

template <size_t N, class T>
inline auto gtAr(T t) {
  return detail::_gtAr<N-1, T>{t};
}

template <class... Ts>
inline auto lt(Ts... ts) {
  return detail::_lt<std::tuple<Ts...>>{std::make_tuple(ts...)};
}

template <size_t N, class T>
inline auto ltAr(T t) {
  return detail::_ltAr<N-1, T>{t};
}

/*!
 * @ingroup algorithms
 * callable that allows all, always returning zero.
 *
 * Useful if some other task is needed like `prll`, `dump`, `cols` etc.
 * */ 
class tautology {
public:
  template <class... Ts>
  bool operator () (Ts... row) {
    return true;
  }  
};

} // namespace ezl

#endif // __FILTERS_EZL_ALGO_H__
