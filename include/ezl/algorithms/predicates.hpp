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

template <class Base> class LogicalOps;

template <class Pred1, class Pred2>
class And : public LogicalOps<And<Pred1, Pred2>> {
public:
  And(Pred1 &&p1, Pred2 &&p2)
      : _pred1{std::forward<Pred1>(p1)}, _pred2{std::forward<Pred2>(p2)} {}
  template <class T> bool operator()(const T &row) {
    return _pred1(row) && _pred2(row);
  }

private:
  Pred1 _pred1;
  Pred2 _pred2;
};

template <class Pred1, class Pred2>
class Or : public LogicalOps<Or<Pred1, Pred2>> {
public:
  Or(Pred1 &&p1, Pred2 &&p2)
      : _pred1{std::forward<Pred1>(p1)}, _pred2{std::forward<Pred2>(p2)} {}
  template <class T> bool operator()(const T &row) {
    return _pred1(row) || _pred2(row);
  }

private:
  Pred1 _pred1;
  Pred2 _pred2;
};

template <class Pred> class Not : public LogicalOps<Not<Pred>> {
public:
  Not(Pred &&p) : _pred{std::forward<Pred>(p)} {}
  template <class T> bool operator()(const T &row) { return !_pred(row); }

private:
  Pred _pred;
};

/*!
 * @ingroup algorithms
 * CRTP inheritance of the class provides logical operators to predicates.
 * */
template <class Base> class LogicalOps {
public:
  template <class Pred> auto operator&&(Pred &&pred) && {
    return And<Base, Pred>{std::move(*(Base *)this), std::forward<Pred>(pred)};
  }
  template <class Pred> auto operator||(Pred &&pred) && {
    return Or<Base, Pred>{std::move(*(Base *)this), std::forward<Pred>(pred)};
  }
  auto operator!() && {
    return Not<Base>{std::move(*(Base *)this)};
  }
  template <class Pred> auto operator&&(Pred &&pred) & {
    return And<Base &, Pred>{*((Base *)this), std::forward<Pred>(pred)};
  }
  template <class Pred> auto operator||(Pred &&pred) & {
    return Or<Base &, Pred>{*((Base *)this), std::forward<Pred>(pred)};
  }
  auto operator!() & {
    return Not<Base &>{*((Base *)this)};
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
template <class Ref> class Eq : public LogicalOps<Eq<Ref>> {
public:
  Eq(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) { return row == _ref; }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * fn. object for equals to a ref element at an index in a gettable param
 * */
template <size_t N, class Ref> class Eqc : public LogicalOps<Eqc<N, Ref>> {
public:
  Eqc(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) {
    return std::get<N>(row) == _ref;
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
template <class Ref> class Gt : public LogicalOps<Gt<Ref>> {
public:
  Gt(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) { return row > _ref; }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * fn. object for greater to a reference element at an index in the tuple
 * */
template <size_t N, class Ref> class Gtc : public LogicalOps<Gtc<N, Ref>> {
public:
  Gtc(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) {
    return std::get<N>(row) > _ref;
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
template <class Ref> class Lt : public LogicalOps<Lt<Ref>> {
public:
  Lt(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) { return row < _ref; }

private:
  Ref _ref;
};

/*!
 * @ingroup algorithms
 * fn. object for greater to a reference element at an index in the tuple
 * */
template <size_t N, class Ref> class Ltc : public LogicalOps<Ltc<N, Ref>> {
public:
  Ltc(Ref r) { _ref = r; }
  template <class T> bool operator()(const T &row) {
    return std::get<N>(row) < _ref;
  }

private:
  Ref _ref;
};

} // namespace ezl::detail

// object generators for relational predicates with template parameter deduction

template <class T> inline auto eq(T t) {
  return detail::Eq<T>{t};
}

template <class T, class V, class... Ts> inline auto eq(T t, V v, Ts... ts) {
  return detail::Eq<std::tuple<T, V, Ts...>>{std::make_tuple(t, v, ts...)};
}

template <size_t N, class T> inline auto eq(T t) {
  return detail::Eqc<N - 1, T>{t};
}

template <class T> inline auto gt(T t) {
  return detail::Gt<T>{t};
}

template <class T, class V, class... Ts> inline auto gt(T t, V v, Ts... ts) {
  return detail::Gt<std::tuple<T, V, Ts...>>{std::make_tuple(t, v, ts...)};
}

template <size_t N, class T> inline auto gt(T t) {
  return detail::Gtc<N - 1, T>{t};
}

template <class T> inline auto lt(T t) {
  return detail::Lt<T>{t};
}

template <class T, class V, class... Ts> inline auto lt(T t, V v, Ts... ts) {
  return detail::Lt<std::tuple<T, V, Ts...>>{std::make_tuple(t, v, ts...)};
}

template <size_t N, class T> inline auto lt(T t) {
  return detail::Ltc<N - 1, T>{t};
}

/*!
 * @ingroup algorithms
 * predicate that always returns true
 *
 * */
class tautology : public detail::LogicalOps<tautology> {
public:
  template <class T> bool operator()(T row) { return true; }
};


} // namespace ezl

#endif // PREDICATES_EZL_ALGO_H
