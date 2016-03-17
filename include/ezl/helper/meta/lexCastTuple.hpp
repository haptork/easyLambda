/*!
 * @file
 * Casting strings to tuples of primitive types including std::array.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef LEXCASTTUPLE_EZL_H
#define LEXCASTTUPLE_EZL_H

#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>  // compile time increase by 2 secs!

namespace ezl {
namespace detail {
namespace meta {

// converts string values into tuple, uses boost-lexical
// throws exception boost::bad_lexical_cast if string can not be converted to
// the tuple datatype. std::array of some type is also supported.
// Used struct because a function will require enable if which is kind of 
// equally complicated.
template <size_t I, class Tup, class T>
struct LexCastImpl {
  LexCastImpl(std::vector<std::string>::iterator& it, Tup &out, bool strict) {
    using T2 = typename std::remove_reference<T>::type;
    --it;
    if (!strict && *it == "") {
      std::get<I>(out) = T2();
    } else {
      std::get<I>(out) = boost::lexical_cast<T2>(*it);
    }
    LexCastImpl<I - 1, Tup, std::tuple_element_t<I - 1, Tup>>(it, out, strict);
  }
};

template <size_t I, class Tup, class T, size_t N>
struct LexCastImpl<I, Tup, std::array<T,N>> {
  LexCastImpl(std::vector<std::string>::iterator &it, Tup &out, bool strict) {
    using T2 = typename std::remove_reference<T>::type;
    for(auto i = int(N-1); i >= 0; i--) {
      --it;
      if (!strict && *it == "") {
        std::get<I>(out)[i] = T2();
      } else {
        std::get<I>(out)[i] = boost::lexical_cast<T2>(*it);
      }
    }
    LexCastImpl<I - 1, Tup, std::tuple_element_t<I - 1, Tup>>(it, out, strict);
  }
};

template <class Tup, class T, size_t N>
struct LexCastImpl<0, Tup, std::array<T, N>> {
  LexCastImpl(std::vector<std::string>::iterator &it, Tup& out, bool strict) {
    using T2 = typename std::remove_reference<T>::type;
    for(auto i = int(N-1); i >= 0; i--) {
      --it;
      if (!strict && *it == "") {
        std::get<0>(out)[i] = T2();
      } else {
        std::get<0>(out)[i] = boost::lexical_cast<T2>(*it);
      }
    }
  }
};

template <class Tup, class T>
struct LexCastImpl<0, Tup, T> {
  LexCastImpl(std::vector<std::string>::iterator &it, Tup &out, bool strict) {
    using T2 = typename std::remove_reference<T>::type;
    --it;
    if (!strict && *it == "") {
      std::get<0>(out) = T2();
    } else {
      std::get<0>(out) = boost::lexical_cast<T2>(*it); // TODO: catch
    }
  }
};

// wrapper functions for string tuple conversion
template <class Tup>
void lexCastTuple(std::vector<std::string> &vstr, Tup &out, bool strict = true) {
  constexpr auto tupSize = std::tuple_size<Tup>::value - 1;
  auto it = std::end(vstr);
  LexCastImpl<tupSize, Tup, std::tuple_element_t<tupSize, Tup>>(it, out, strict);
};

} // namespace meta
} // namespace detail 
} // namespace ezl

#endif // _LEXCASTTUPLE_EZL_H__
