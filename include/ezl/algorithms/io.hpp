/*!
 * @file
 * Function objects for adding basic io functionality (rises & dumps)
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
 * rise<int, char, float>(filename)
 * .filter<1, 2>(dumpMem(buffer)).run()
 * 
 * auto buf = dumpMem<int, char>();
 * rise<int char>(filename)
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
  /*!
   * ctor that uses external buffer.
   * */
  _dumpMem(std::vector<T>& buffer) : _buffer{buffer} {}
  /*!
   * ctor that uses interanl buffer.
   * */
  _dumpMem() : _buffer{_internalBuffer} {}
  /*!
   * 
   * */
  bool operator () (T row) {
    _buffer.emplace_back(std::move(row));
    return true;
  }
  /*!
   * Get the buffer reference.
   * */
  std::vector<T>& buffer() {
    return _buffer;
  }
  /*!
   * Reset the buffer by clearing it.
   * */
  auto& clear() & {
    _buffer.clear();
    return *this;
  }
  /*!
   * Reset the buffer by clearing it.
   * */
  auto clear() && {
    _buffer.clear();
    return std::move(*this);
  }
private:
  std::vector<T> _internalBuffer;
  std::vector<T>& _buffer;
};
} // namespace detail

/*!
 * ctor function for DumpMem that uses external buffer.
 * */
template <class T> auto dumpMem(T& buffer) {
  return detail::_dumpMem<typename T::value_type>{buffer};
}

/*!
 * ctor function for DumpMem that uses internal buffer.
 * */
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
 * rise(ezl::fromFileNames("*.jpg"))
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
  /*!
   * called by rise to pass process information before running of the dataflow
   * */
  auto operator () (const int& pos, const std::vector<int>& procs) { 
    _fnames = detail::vglob(_fpat, _limitFiles);
    if(!_fnames.empty() && _isSplit) _share(pos, procs.size());
  }
  /*!
   * called by rise for pulling data.
   * */
  std::vector<std::string> operator () () {
    return std::move(_fnames);
  }
  /*!
   * whether to split the files among available processes.
   * */
  auto split(bool isSplit = true) && {
    _isSplit = isSplit;
    return std::move(*this);
  }
  /*!
   * whether to split the files among available processes.
   * */
  auto& split(bool isSplit = true) & {
    _isSplit = isSplit;
    return *this;
  }
  auto limitFiles(size_t count) && {
    _limitFiles = count;
    return std::move(*this);
  }
  auto& limitFiles(size_t count) & {
    _limitFiles = count;
    return *this;
  }
  /*!
   * reset file pattern
   * */
  auto reset(std::string fpat) && {
    _fpat = fpat;
    return std::move(*this);
  }
  /*!
   * reset file pattern
   * */
  auto& reset(std::string fpat) & {
    _fpat = fpat;
    return *this;
  }
private:
  void _share(int pos, size_t total) {
    auto len = _fnames.size();
    auto share = size_t(len / total);
    if (share == 0) share = 1;
    size_t rBeginFile = share * pos;
    size_t rEndFile = (share * (pos + 1)) - 1;
    if (rEndFile > (len - 1) || size_t(pos) == total - 1) {
      rEndFile = len - 1;
    }
    if (rBeginFile > (len - 1)) {
      _fnames.clear();
    } else {
      rEndFile -= rBeginFile;
      for (size_t i = 0; i <= rEndFile; i++) {
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
 * ezl::rise(ezl::fromMem({1,2,3})).build();
 *
 * std::vector<std::tuple<int, char>> a;
 * a.push_back(make_tuple(4, 'c');
 * ezl::rise(ezl::fromMem(a)).build();
 *
 * ezl::rise(ezl::fromMem(std::array<float, 2> {4., 2.}}, false))
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
  FromMem(const T &source, bool isShard)
      : _isSplit{isShard} {
    _isVal = false;
    _vDataHandle = &source;
  }
  /*!
  * ctor for rvalue source. The params are same as in lvalue ctor.
  * */
  FromMem(const T &&source, bool isShard = false)
      : _isSplit{isShard} {
    _isVal = true;
    _vDataVal = std::move(source);
    _vDataHandle = &_vDataVal;
  }
  /*!
  * change buffer to get data from a variable (lvalue)
  * */
  auto buffer(const T &source) && {
    _isVal = false;
    _vDataHandle = &source;
    return std::move(*this);
  }
  /*!
  * change buffer to get data from a variable (lvalue)
  * */
  auto& buffer(const T &source) & {
    _isVal = false;
    _vDataHandle = &source;
    return *this;
  }
  /*!
  * change buffer to get data from an instant list (rvalue)
  * */
  auto buffer(const T &&source) && {
    _isVal = true;
    _vDataVal = std::move(source);
    _vDataHandle = &_vDataVal;
    return std::move(*this);
  }
  /*!
  * change buffer to get data from an instant list (rvalue)
  * */
  auto& buffer(const T &&source) & {
    _isVal = true;
    _vDataVal = std::move(source);
    _vDataHandle = &_vDataVal;
    return *this;
  }
  /*!
   * whether to split the data among available processes.
   * */
  auto split(bool isSplit = true) && {
    _isSplit = isSplit;
    return std::move(*this);
  }
  /*!
   * whether to split the data among available processes.
   * */
  auto& split(bool isSplit = true) & {
    _isSplit = isSplit;
    return *this;
  }
  /*!
   * called by rise to pass process information before running of the dataflow
   * */
  auto operator () (const int& pos, const std::vector<int>& procs) { 
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
  /*!
   * called by rise for pulling data.
   * */
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

/*!
 * @ingroup algorithms
 * function object for generating a range either [0,n) or [m, n].
 * The range can be optinally split among processes with each getting
 * (m-n)/p numbers (last one gets till n-1), sequentially.
 *
 * Example usage:
 * @code
 * rise(iota(7))  // 6 total, if run on two procs , first process gets [0, 1, 2]
 * and second one gets [3, 4, 5, 6].
 *
 * rise(iota(3,5).split(false)).build();  // [3,4] to each process
 * @endcode
 *
 * */
template<class T>
class _iota {
public:
  _iota(T times, bool isSplit)
      : _first{0}, _last{times}, _isSplit{isSplit} {}
  _iota(T first, T last, bool isSplit)
      : _first{first}, _last{last}, _isSplit{isSplit} {}
  /*!
   * called by rise for pulling data.
   * */
  auto operator () () {
    _cur++;
    return make_pair(std::tuple<T>{_cur - 1}, (_cur - 1) < _max);
  }
  /*!
   * called by rise to pass process information before running of the dataflow
   * */
  auto operator () (const int& pos, const std::vector<int>& procs) { 
    if(_isSplit) {
      _share(pos, procs.size());
    } else {
      _cur = _first;
      _max = _last;
    }
  }
  /*!
   * reset the range to new last value starting from 0
   * */
  auto reset(T last) && {
    _first = 0;
    _last = last;
    return std::move(*this);
  }
  /*!
   * reset the range to new last value starting from 0
   * */
  auto& reset(T last) & {
    _first = 0;
    _last = last;
    return *this;
  }
  /*!
   * reset the range to new first and last
   * */  
  auto reset(T first, T last) && {
    _first = first; 
    _last = last;
    return std::move(*this);
  }
  /*!
   * reset the range to new first and last
   * */  
  auto& reset(T first, T last) & {
    _first = first; 
    _last = last;
    return *this;
  }
  /*!
   * whether to split the data among available processes.
   * */
  auto split(bool isSplit = true) && {
    _isSplit = isSplit;
    return std::move(*this);
  }
  /*!
   * whether to split the data among available processes.
   * */
  auto& split(bool isSplit = true) & {
    _isSplit = isSplit;
    return *this;
  }
private:
  void _share(int pos, int total) {
    auto len = _last - _first;
    auto share = T(len / total);
    if (share == 0) share = 1;
    _cur = share * pos + _first;
    _max = (share * (pos + 1)) + _first;
    if (_max > _last || pos == total - 1) {
      _max = _last;
    }
  }
  T _max;
  T _cur;
  T _first;
  T _last;
  bool _isSplit;
};
} // namespace detail

// a function to help in ctoring fromMem with template parameter deduction.
template <class T>
auto fromMem(T&& source, bool isSplit = false) {
  using cleanT = std::decay_t<T>;
  return detail::FromMem<cleanT> {std::forward<T>(source), isSplit};
}

// a function to help in ctoring fromMem with template parameter deduction.
template <class T>
auto fromMem(std::initializer_list<T> source, bool isSplit = false) {
  return detail::FromMem<std::vector<T>> {std::move(source), isSplit};
}

// a function to help in ctoring iota with template parameter deduction.
template <class T>
auto iota(T times, bool isSplit = true) {
  return detail::_iota<T> {times, isSplit};
}

// a function to help in ctoring iota with template parameter deduction.
template <class T>
auto iota(T first, T last, bool isSplit = true) {
  return detail::_iota<T> {first, last, isSplit};
}

/*!
 * @ingroup algorithms
 * function object for calling the next unit a number of times without any columns.
 * The number of times can be optionally partitioned among available processes.
 *
 * Example usage:
 * @code
 * rise(kick(60))  // 60 total, if run on two procs 30 each
 * .map([]() { return rand(); })
 * .build();
 *
 * rise(kick(60, false)).build();  // 60 each, if run on two procs 120 total
 * @endcode
 *
 * */
class kick {
public:
  /*!
   * called by rise for pulling data.
   * */
  kick(size_t times = 1, bool isSplit = true)
      : _times{times}, _isSplit{isSplit} {}
  /*!
   * called by rise for pulling data.
   * */
  auto operator () () {
    ++_cur;
    return make_pair(std::tuple<>{}, (_cur <= _max));
  }
  /*!
   * called by rise to pass process information before running of the dataflow
   * */
  auto operator () (const int& pos, const std::vector<int>& procs) { 
    _max = (_isSplit) ? _share(pos, procs.size()) : _times;
    _cur = 0;
  }
  /*!
   * reset the number of times to new value
   * */  
  auto reset(size_t times) && {
    _times = times;
    return std::move(*this);
  }
  /*!
   * reset the number of times to new value
   * */  
  auto& reset(size_t times) & {
    _times = times;
    return *this;
  }
  /*!
   * whether to split the data among available processes.
   * */
  auto split(bool isSplit = true) && {
    _isSplit = isSplit;
    return std::move(*this);
  }
  /*!
   * whether to split the data among available processes.
   * */
  auto& split(bool isSplit = true) & {
    _isSplit = isSplit;
    return *this;
  }
private:
  size_t _share(int pos, int total) {
    auto share = size_t(_times / total);
    auto res = share;
    if (pos == total - 1) {
      res = _times - share*pos;
    }
    return res;
  }
  size_t _times;
  size_t _cur;
  size_t _max;
  bool _isSplit;
};

} // namespace ezl

#endif // !IO_EZL_ALGO_H
