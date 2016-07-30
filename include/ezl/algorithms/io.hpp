/*!
 * @file
 * Function objects for adding basic io functionality (loads & dumps)
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef IO_EZL_ALGO_H
#define IO_EZL_ALGO_H

#include <tuple>
#include <vector>
#include <string>
#include <algorithm>

#include <ezl/helper/vglob.hpp>

namespace ezl {
namespace detail {

/*!
 * @ingroup algorithms
 * function object for dumping to memory, can be used in a filter unit.
 *
 * Example usage:
 * @code 
 * vector<tuple<int, char>> buffer;
 * load<int, char, float>(filename)
 * .filter<1, 2>(dumpMem(buffer)).run()
 * 
 * auto buf = dumpMem<int, char>();
 * load<int char>(filename)
 * .filter<1, 2>(buf).run()
 * 
 * auto& data = buf.buffer();
 * @endcode
 *
 * Consider using `runResult()` instead of filter and run()
 * to get the buffer in return result of a pipeline.
 * */
template <class T>
class _dumpMem {
public:
  _dumpMem(std::vector<T>& buffer) : _buffer{buffer} {}
  _dumpMem() : _buffer{_internalBuffer} {}

  bool operator () (T row) {
    _buffer.emplace_back(std::move(row));
    return true;
  }

  std::vector<T>& buffer() {
    return _buffer;
  }
private:
  std::vector<T> _internalBuffer;
  std::vector<T>& _buffer;
};
} // namespace detail

template <class T> auto dumpMem(T& buffer) {
  return detail::_dumpMem<typename T::value_type>{buffer};
}

template <class... Ts> auto dumpMem() {
  return detail::_dumpMem<std::tuple<Ts...>>{};
}

/*!
 * @ingroup algorithms
 * function object for loading file names from glob pattern, optionally parallely
 * distributed among processes.
 *
 * Example usage: 
 * @code
 * load(ezl::fromFileNames("*.jpg"))
 * .map([](string imageFile) [] { return cv::imread(imageFile); ).build()
 * @endcode
 *
 * */
class fromFileNames {
public:
  /*!
   * ctor
   * @param fpat file glob pattern, such as "*.txt"
   * @param isSplit if set true the the file list is partitioned equally
   *                among the available processes, else all the processes
   *                work on full list.
   * */
  fromFileNames(std::string fpat, bool isSplit = true)
      : _fpat{fpat}, _isSplit{isSplit} {}

  auto operator () (int pos, std::vector<int> procs) { 
    _fnames = detail::vglob(_fpat, _limitFiles);
    if(!_fnames.empty() && _isSplit) _share(pos, procs.size());
  }

  std::vector<std::string> operator () () {
    return std::move(_fnames);
  }

  auto split(bool isSplit = true) {
    _isSplit = isSplit;
    return std::move(*this);
  }

  auto limitFiles(size_t count) {
    _limitFiles = count;
    return std::move(*this);
  }

private:
  void _share(int pos, size_t total) {
    auto len = _fnames.size();
    auto share = size_t(len / total);
    if (share == 0) share = 1;
    size_t rBeginFile = share * pos;
    size_t rEndFile = (share * (pos + 1)) - 1;
    if (rEndFile > (len - 1) || pos == total - 1) {
      rEndFile = len - 1;
    }
    // now removing not required _fnames from the list
    if (rBeginFile > (len - 1)) {
      _fnames.empty();
    } else {
      rEndFile -= rBeginFile;
      for (auto i = 0; i <= rEndFile; i++) {
        _fnames[i] = _fnames[i + rBeginFile];
      }
      rBeginFile = 0;
      _fnames.resize(rEndFile + 1);
    }
  }

  std::string _fpat;
  bool _isSplit;
  size_t _limitFiles{0};
  std::vector<std::string> _fnames;
};

/*!
 * @ingroup algorithms
 * function object for loading from memory, optionally parallely distributed among
 * processes.
 *
 * @param T any container type.
 *
 * Example usage:
 * @code
 * ezl::load(ezl::fromMem({1,2,3})).build();
 *
 * std::vector<std::tuple<int, char>> a;
 * a.push_back(make_tuple(4, 'c');
 * ezl::load(ezl::fromMem(a)).build();
 *
 * ezl::load(ezl::fromMem(std::array<float, 2> {4., 2.}}, false))
 * @endcode
 *
 * */
namespace detail {
template <class T>
class FromMem {
public:
  using I = typename T::value_type;
  /*!
  * ctor
  * @param source a vector of types.
  * @param isSplit if set true the the file list is partitioned equally
  *                among the available processes, else all the processes
  *                work on full list.
  * */
  FromMem(const T &source, bool isShard = false)
      : _isSplit{isShard} {
    _isVal = false;
    _vDataHandle = &source;
  }

  /*!
  * ctor for rvalue source . The params are same as in lvalue ctor.
  * */
  FromMem(const T &&source, bool isShard = false)
      : _isSplit{isShard} {
    _isVal = true;
    _vDataVal = std::move(source);
    _vDataHandle = &_vDataVal;
  }

  auto buffer(const T &source) {
    _isVal = false;
    _vDataHandle = &source;
    return std::move(*this);
  }

  auto buffer(const T &&source) {
    _isVal = true;
    _vDataVal = std::move(source);
    _vDataHandle = &_vDataVal;
    return std::move(*this);
  }

  auto split(bool isSplit = true) {
    _isSplit = isSplit;
    return std::move(*this);
  }

  auto operator () (int pos, std::vector<int> procs) { 
    _more = true;
    if (_isVal) {
      _vDataHandle = &_vDataVal;
    }
    if(!_isSplit) {
      _cur = std::begin(*_vDataHandle);
      _last = std::end(*_vDataHandle);
      return;
    }
    auto edges = _share(pos, procs.size(), _vDataHandle->size());
    _cur = std::next(std::begin(*_vDataHandle), edges[0]);
    _last = std::next(std::begin(*_vDataHandle), edges[1] - 1);
  }

  // A variant / optional will be much better than tie
  int i = 0;
  auto operator () () {
    auto res = std::tie((*_cur), _more); // no copies
    ++_cur;
    if (_cur > _last) { 
      _more = false;
      --_cur;
    }
    return res;
  }

private:
  auto _share(int pos, int total, size_t len) {
    auto share = len / total;
    if (share == 0) share = 1;
    auto first = share * pos;
    auto last = share * (pos + 1); 
    if (first > len) { 
      first = len; 
    }
    if (last > len || pos == total - 1) {
      last = len;
    }
    return std::array<size_t, 2> {{first, last+1}};
  }

  T _vDataVal;
  const T* _vDataHandle;
  bool _isSplit;
  typename T::const_iterator _cur;
  typename T::const_iterator _last;
  bool _more {true};
  bool _isVal;
};
} // namespace detail

// a function to help in template parameter deduction without specifying.
// why const does not work with rvalue? TODO
template <class T>
auto fromMem(T&& source, bool isSplit = false) {
  using cleanT = std::decay_t<T>;
  return detail::FromMem<cleanT> {std::forward<T>(source), isSplit};
}

template <class T>
auto fromMem(std::initializer_list<T> source, bool isSplit = false) {
  return detail::FromMem<std::vector<T>> {std::move(source), isSplit};
}

/*!
 * @ingroup algorithms
 * function object for calling the next unit a number of times without any columns.
 * The number of times can be optionally partitioned among available processes.
 *
 * Example usage:
 * @code
 * load(kick(60))  // 60 total, if run on two procs 30 each
 * .map([]() { return rand(); })
 * .build();
 *
 * load(kick(60, false)).build();  // 60 each, if run on two procs 120 total
 * @endcode
 *
 * */
class kick {
using int64 = long long; 
public:
  kick(int64 times = 1, bool isSplit = true) : _times{times}, _isSplit{isSplit} {}

  auto operator () () {
    _times--;
    return make_pair(std::tuple<>{}, (_times>=0));
  }

  auto operator () (int pos, std::vector<int> procs) { 
    if(_isSplit) {
      _times = _share(pos, procs.size());
    }
  }

  auto times(int64 t) {
    _times = t;
    return std::move(*this);
  }

  auto split(bool isSplit = true) {
    _isSplit = isSplit;
    return std::move(*this);
  }
private:
  int64 _share(int pos, int total) {
    auto share = int64(_times / total);
    auto res = share;
    if (pos == total - 1) {
      res = _times - share*pos;
    }
    return res;
  }

  int64 _times;
  bool _isSplit;
};

} // namespace ezl

#endif // !IO_EZL_ALGO_H
