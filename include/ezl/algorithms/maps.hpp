/*!
 * @file
 * Function objects for basic functionality with `map`
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef MAPS_EZL_ALGO_H
#define MAPS_EZL_ALGO_H

#include <tuple>
#include <array>

namespace ezl {

/*!
 * @ingroup algorithms
 * function object for merging many types into an array, or an array and
 * types or two arrays. 
 *
 * Example usage: 
 * @code
 * ezl::load<int, int, float>(argv[1])
 * .map<1,2>(ezl::mergeAr()).colsTransform().build()
 *
 * ezl::load<int, float, array<int, 2>(argv[1])
 * .map<3,1>(ezl::mergeAr()).colsTransform().build()
 *
 * ezl::load<array<int, 3>, array<int, 2>(argv[1])
 * .map(ezl::mergeAr()).colsTransform().build()
 * @endcode
 *
 * */
class mergeAr {
private:
  template<typename T> struct isArray : public std::false_type {};
  template<typename T, size_t N>
  struct isArray<std::array<T, N>> : public std::true_type {};
public:

  template <class T, size_t N, class... Ts>
  auto operator () (const std::array<T, N> &t1, const Ts&... ts) {
    constexpr auto size = sizeof...(Ts) + N; 
    std::array<T, size> res;
    for (size_t i = 0; i < N; i++) {
      res[i] = t1[i];
    }
    auto later = _toArray(std::make_tuple(ts...),
                          std::make_index_sequence<sizeof...(Ts)>{});
    for (size_t i = N, j = 0; i < size; i++, j++) {
      res[i] = later[j];
    }
    return res;
  }

  template <class T, class U, size_t N, size_t M>
  auto operator () (const std::array<T, N>& t1, const std::array<U, M>& t2) {
    std::array<T, M+N> res;
    for (size_t i = 0; i < N; i++) {
      res[i] = t1[i];
    }
    for (size_t i = N, j = 0; j < M; i++, j++) {
      res[i] = t2[j];
    }
    return res;
  }

  // T1, T2 to take arguments as expanded tuple
  template <class T1, class T2, class... Ts>
  typename std::enable_if<!isArray<T1>{}, std::array<T1, sizeof...(Ts) + 2>>::type
  operator () (const T1& t1, const T2& t2, Ts... t) {
    constexpr auto size = sizeof...(Ts) + 2;
    return _toArray(std::make_tuple(t1, t2, t...), std::make_index_sequence<size>{});
  }

private:
  template <class I, class... Is, std::size_t... index>
  auto _toArray(const std::tuple<I, Is...> &tup, std::index_sequence<index...>) {
    return std::array<I, sizeof...(Is) + 1>{{I(std::get<index>(tup))...}};
  }
};


/*!
 * @ingroup algorithms
 * function object for exploding an array to different columns.
 *
 * Example usage: 
 * @code
 *
 * ezl::load<array<int, 3>, int>(argv[1])
 * .map<1>(ezl::explodeAr()).colsTransform().build()
 * @endcode
 *
 * */
class explodeAr {
public:
  template <class T , size_t N>
  auto operator () (const std::array<T, N>& ar) {
    return _explode(ar, std::make_index_sequence<N>{});
  }
private:
  template <class T, std::size_t... index>
  auto _explode(const T& ar, std::index_sequence<index...>) {
    return std::make_tuple(std::get<index>(ar)...);
  }
};

/*!
 * @ingroup algorithms
 * function object for adding a serial number to the rows.
 * Please note that the serial number begins indipendently in each process.
 *
 * */
namespace detail {
template <class T>
class _serialNumber {
public:
  _serialNumber(T init) {
    count = init;
  } 

  template <class U>
  auto operator () (const U&) {
    return count++;  
  }
private:
  T count;
};
} // namespace detail

template <class T = int>
auto serialNumber(T init = 1) {
  return detail::_serialNumber<T>{init};
} 


/*!
 * @ingroup algorithms
 * function object for buffering before sending to next unit 
 * possibly in different process.
 * 
 * */
namespace detail {
template <class U>
class _buffer {
public:
  _buffer(size_t size) : _size{size} {
    _buf.reserve(_size); 
  }

  auto operator () (const U& u) {
    if(_buf.size() < _size) {
      if(_buf.empty()) _buf.reserve(_size);
      _buf.push_back(u);
      return nil;
    }
    return std::move(_buf);
  }
private:
  std::vector<U> _buf;
  std::vector<U> nil;
  size_t _size;
};
}

template <class... Ts>
auto buffer(size_t size) {
  return detail::_buffer<std::tuple<Ts...>>{size};
}

}  // namespace ezl

#endif // !MAPS_EZL_ALGO_H
