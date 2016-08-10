/*!
 * @file
 * Function objects for relational predicates that work with logical operators
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef PREDICATES_EZL_ALGO_H
#define PREDICATES_EZL_ALGO_H

#include <tuple>

namespace ezl {
namespace detail {

template <class Base> class logicalOps;

template <class Pred1, class Pred2>
class _and : public logicalOps<_and<Pred1, Pred2>> {
public:
  _and(Pred1 &&p1, Pred2 &&p2)
      : _pred1{std::forward<Pred1>(p1)}, _pred2{std::forward<Pred2>(p2)} {}
  template <class T> bool operator()(const T &row) {
    return _pred1(row) && _pred2(row);
  }

private:
  Pred1 _pred1;
  Pred2 _pred2;
};

template <class Pred1, class Pred2>
class _or : public logicalOps<_or<Pred1, Pred2>> {
public:
  _or(Pred1 &&p1, Pred2 &&p2)
      : _pred1{std::forward<Pred1>(p1)}, _pred2{std::forward<Pred2>(p2)} {}
  template <class T> bool operator()(const T &row) {
    return _pred1(row) || _pred2(row);
  }

private:
  Pred1 _pred1;
  Pred2 _pred2;
};

template <class Pred> class _not : public logicalOps<_not<Pred>> {
public:
  _not(Pred &&p) : _pred{std::forward<Pred>(p)} {}
  template <class T> bool operator()(const T &row) { return !_pred(row); }

private:
  Pred _pred;
};

/*!
 * @ingroup algorithms
 * CRTP inheritance of the class provides logical operators to predicates.
 * */
template <class Base> class logicalOps {
public:
  template <class Pred> auto operator&&(Pred &&pred) && {
    return _and<Base, Pred>{std::move(*(Base *)this), std::forward<Pred>(pred)};
  }
  template <class Pred> auto operator||(Pred &&pred) && {
    return _or<Base, Pred>{std::move(*(Base *)this), std::forward<Pred>(pred)};
  }
  auto operator!() && {
    return _not<Base>{std::move(*(Base *)this)};
  }
  template <class Pred> auto operator&&(Pred &&pred) & {
    return _and<Base &, Pred>{*((Base *)this), std::forward<Pred>(pred)};
  }
  template <class Pred> auto operator||(Pred &&pred) & {
    return _or<Base &, Pred>{*((Base *)this), std::forward<Pred>(pred)};
  }
  auto operator!() & {
    return _not<Base &>{*((Base *)this)};
  }
};

/*!
 * @ingroup algorithms
 * fn. object for equals to a reference number.
 *
 * Can be used to filter rows based on equality with a refence.
 *
 * E.g.:
 * load<int, char, float>(filename)
 * .filter<2>(eq('c'))
 *
 * The above example filters all the rows that have 2nd column as 'c'.
 * */
template <class Ref> class _eq : public logicalOps<_eq<Ref>> {
public:
  _eq(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) { return row == _ref; }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * fn. object for equals to a reference element at an index in the tuple
 * */
template <size_t N, class Ref> class _eqc : public logicalOps<_eqc<N, Ref>> {
public:
  _eqc(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) {
    return std::get<N>(row) == _ref;
  }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * fn. object for equals to a reference number and an index in the array
 *
 * Can be used to filter rows having array, based on equality with a refence.
 * E.g.:
 * load<array<int, 3>>(filename)
 * .filter(eqAr<2>(0))
 *
 * The above example filters all the rows where second element of the
 * array is equal to 0.
 *
 * Can be used with any types that support std::get such as pair, array...
 * */
template <size_t N, class Ref> class _eqAr : public logicalOps<_eqAr<N, Ref>> {
public:
  _eqAr(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) {
    return std::get<N>(std::get<0>(row)) == _ref;
  }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * fn. object for greater than a reference number
 *
 * Usage is same as `eq`
 * */
template <class Ref> class _gt : public logicalOps<_gt<Ref>> {
public:
  _gt(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) { return row > _ref; }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * fn. object for greater to a reference element at an index in the tuple
 * */
template <size_t N, class Ref> class _gtc : public logicalOps<_gtc<N, Ref>> {
public:
  _gtc(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) {
    return std::get<N>(row) > _ref;
  }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * fn. object for greater than a reference number and an index in the array
 *
 * Usage is same as `eqAr`.
 * */
template <size_t N, class Ref> class _gtAr : public logicalOps<_gtAr<N, Ref>> {
public:
  _gtAr(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) {
    return std::get<N>(std::get<0>(row)) > _ref;
  }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * fn. object for less than a reference number
 *
 * Usage is same as `eq`
 * */
template <class Ref> class _lt : public logicalOps<_lt<Ref>> {
public:
  _lt(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) { return row < _ref; }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * fn. object for greater to a reference element at an index in the tuple
 * */
template <size_t N, class Ref> class _ltc : public logicalOps<_ltc<N, Ref>> {
public:
  _ltc(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) {
    return std::get<N>(row) > _ref;
  }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * fn. object for less than a reference number and an index in the array
 *
 * Usage is same as `eqAr`.
 * */
template <size_t N, class Ref> class _ltAr : public logicalOps<_ltAr<N, Ref>> {
public:
  _ltAr(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) {
    return std::get<N>(std::get<0>(row)) < _ref;
  }

private:
  Ref _ref;
};

} // namespace ezl::detail

// functions for ctoring the predicates with template parameter deduction
template <class... Ts> inline auto eq(Ts... ts) {
  return detail::_eq<std::tuple<Ts...>>{std::make_tuple(ts...)};
}

template <size_t N, class T> inline auto eq(T t) {
  return detail::_eqc<N - 1, T>{t};
}

template <size_t N, class T> inline auto eqAr(T t) {
  return detail::_eqAr<N - 1, T>{t};
}

template <class... Ts> inline auto gt(Ts... ts) {
  return detail::_gt<std::tuple<Ts...>>{std::make_tuple(ts...)};
}

template <size_t N, class T> inline auto gt(T t) {
  return detail::_gtc<N - 1, T>{t};
}

template <size_t N, class T> inline auto gtAr(T t) {
  return detail::_gtAr<N - 1, T>{t};
}

template <class... Ts> inline auto lt(Ts... ts) {
  return detail::_lt<std::tuple<Ts...>>{std::make_tuple(ts...)};
}

template <size_t N, class T> inline auto lt(T t) {
  return detail::_ltc<N - 1, T>{t};
}

template <size_t N, class T> inline auto ltAr(T t) {
  return detail::_ltAr<N - 1, T>{t};
}

/*!
 * @ingroup algorithms
 * fn. object that allows all, always returning zero.
 *
 * Useful if some other task is needed like `prll`, `dump`, `cols` etc.
 * */
class tautology {
public:
  template <class... Ts> bool operator()(Ts... row) { return true; }
};

} // namespace ezl

#endif // PREDICATES_EZL_ALGO_H
