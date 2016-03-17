/*!
 * @file
 * class ReduceAll
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef REDUCEALL_EZL_H
#define REDUCEALL_EZL_H

#include <tuple>
#include <vector>

#include "boost/unordered_map.hpp"

#include "ezl/pipeline/Link.hpp"
#include "ezl/helper/meta/coherentVector.hpp"
#include "ezl/helper/meta/slctTuple.hpp"
#include "ezl/helper/meta/typeInfo.hpp"
#include "ezl/helper/meta/funcInvoke.hpp"

namespace ezl {
namespace detail {
/*!
 * @ingroup mapreduce
 * ReduceAll unit for reducing grouped rows vector into zero, one or many rows.
 * 
 * The UDF can have params of type: tuple of key columns followed by vector of
 * tupleof value columns or key columns followed by vectors of value columns.
 *
 * Key and value columns can be selected by indices. 
 *
 * The output from the unit can be selected from key and UDF output columns.
 *
 * UDF may return a single value for returning a single column or a tuple
 * for multiple columns. 
 * A vector is to be returned for returning variable number of rows for each
 * input row.
 *
 * See examples for using with builders or unittests for direct use.
 *
 * */
template <class TypeInfo, class HashScheme>
              //typename meta::ReduceAllTypes<I, Kslct, Vslct, Func, Oslct>>
struct ReduceAll : public Link<typename TypeInfo::itype, typename TypeInfo::otype> {
private:
  struct EqWrapper {
  public: 
    template <class T, class U>
    bool operator () (const T& a, const U& b) const { return a==b; }
  };
public:
  using itype = typename TypeInfo::itype;
  using Func = typename TypeInfo::Func;
  using ktype = typename TypeInfo::ktype;
  using buftype = typename TypeInfo::buftype;
  using otype = typename TypeInfo::otype;
  using maptype = boost::unordered_map<ktype, buftype, HashScheme>;

  static constexpr int osize = std::tuple_size<otype>::value;

  ReduceAll(Func m, bool ordered, bool adjacent, int bunchSize)
      : _func(m), _ordered(ordered), _adjacent(adjacent),
        _bunchSize(bunchSize) {}

  virtual void dataEvent(const itype &data) final override {
    auto curKey = meta::slctTupleRef(data, _kslct);
    auto curVal = meta::slctTuple(data, _vslct);
    auto it = _index.find(curKey, _hash, _eq);
    if (it == std::end(_index)) {
      buftype v;
      meta::coherentPush(v, curVal);
      it = _index.emplace_hint(it, ktype(curKey), v);
    } else {
      meta::coherentPush(it->second, curVal);
    }
    if (_bunchSize > 0) { _processBunched(it); }
    if (_ordered) { _processOrdered(it); }
  } 

private:
  virtual void _dataEnd(int ) final override {
    for (auto &it : _index) {
      if (!_adjacent || meta::coherentSize(it.second) < _bunchSize - 1) {
        _processReduceAll(it.first, it.second);
      }
    }
    _index.clear();
    _first = false;
  }

  void _processReduceAll(const ktype &key, const buftype &buffer) {
    if (meta::coherentSize(buffer) == 0) return;
    callEm<decltype(meta::invokeReduceAll(_func, key, buffer))>(key, buffer);
  }
  
  template <class T>
  auto callEm(const ktype &key, const buftype &buffer, typename std::enable_if<meta::isVector<T>{}>::type* dummy = 0) {
    auto funOut = meta::invokeReduceAll(_func, key, buffer);
    if (Link<itype, otype>::next().empty()) return;
    std::vector<otype> res;
    res.reserve(funOut.size());
    for (auto &it : funOut) {
      res.emplace_back(meta::slctTupleRef(meta::tieTup(key, it), _oslct));
    }
    for (auto &it : Link<itype, otype>::next()) {
      it.second->dataEvent(res);
    }
  }
  
  template <class T>
  auto callEm(const ktype &key, const buftype &buffer, typename std::enable_if<!meta::isVector<T>{}>::type* dummy = 0) {
    auto funOut = meta::invokeReduceAll(_func, key, buffer);
    if (Link<itype, otype>::next().empty()) return;
    auto result = meta::slctTupleRef(meta::tieTup(key, funOut), _oslct);
    for (auto &it : Link<itype, otype>::next()) {
      it.second->dataEvent(result);
    }
  }

  bool _processBunched(const typename maptype::iterator& it) {
    auto size = meta::coherentSize(it->second);
    if (size >= _bunchSize) {
      _processReduceAll(it->first, it->second);
      if (_adjacent) {
        meta::coherentPopFront(it->second); // TODO: optimize pop
      } else {
        meta::coherentClear(it->second);
      }
      return true;
    }
    return false;
  }

  void _processOrdered(const typename maptype::iterator& it) {
    if (_first) {
      _first = false;
      _preIt = it;
    } else if (_preIt->first != it->first) {
      _processReduceAll(_preIt->first, _preIt->second);
      _index.erase(_preIt);
      _preIt = it;
    }
  }

private:
  Func _func;
  bool _ordered{false};
  bool _adjacent{false};
  int _bunchSize{0};
  maptype _index;
  HashScheme _hash;
  EqWrapper _eq;
  bool _first{true};
  typename maptype::iterator _preIt;
  typename TypeInfo::Kslct _kslct;
  typename TypeInfo::Vslct _vslct;
  typename TypeInfo::Oslct _oslct;
};
}
} // namespace ezl ezl::detail

#endif // ! REDUCEALL_EZL_H
