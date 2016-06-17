/*!
 * @file
 * class reduce.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef REDUCE_EZL_H
#define REDUCE_EZL_H

#include <tuple>
#include <vector>

#include <boost/unordered_map.hpp>

#include <ezl/pipeline/Link.hpp>
#include <ezl/helper/meta/funcInvoke.hpp>
#include <ezl/helper/meta/slctTuple.hpp>
#include <ezl/helper/meta/typeInfo.hpp>

namespace ezl {
namespace detail {

/*!
 * @ingroup mapreduce
 * Reduce unit for reducing grouped rows piecemeal as they stream, returning
 * zero, one or many new rows. 
 *
 * The UDF can have params of type: tuple of or free types for key, value and
 * result columns. 
 *
 * Key and value columns can be selected by specifying indices. 
 *
 * The output from the unit can be selected from key and UDF output columns.
 * 
 * UDF may return a single value for a column or tuple for multiple columns.
 * A vector is to be returned for returning variable number of rows for each
 * input row.
 *
 * See examples for using with builders or unittests for direct use.
 *
 * */
template <class TypeInfo, class HashScheme>
struct Reduce : public Link<typename TypeInfo::itype, typename TypeInfo::otype> {
private:
  struct EqWrapper {
  public: 
    template <class T, class U>
    bool operator () (const T& a, const U& b) const { return a == b; }
  };
public:
  using itype = typename TypeInfo::itype;
  using Kslct = typename TypeInfo::K;
  using Vslct = typename TypeInfo::V;
  using F = typename TypeInfo::Func;
  using FO = typename TypeInfo::FO;
  using Oslct = typename TypeInfo::Oslct;
  using ktype = typename TypeInfo::ktype;
  using otype = typename TypeInfo::otype;
  using maptype =  boost::unordered_map<ktype, FO, HashScheme>;

  static constexpr int osize = std::tuple_size<otype>::value;

  Reduce(F f, FO val, bool scan, bool order) : _func(f), _initVal(val), _scan(scan), _ordered(order) {}

  virtual void dataEvent(const itype &data) final override {
    auto curKey = meta::slctTupleRef(data, Kslct{});
    auto curVal = meta::slctTupleRef(data, Vslct{});
    auto it = _index.find(curKey, _hash, _eq);

    if(TypeInfo::isRefRes) {
      if (it != std::end(_index)) {
        decltype(auto) x = meta::invokeReduce(_func, curKey, curVal, it->second);
        if (&x != &it->second) {
          it->second = x;
        }
      } else {
        it = _index.emplace_hint(it, ktype(curKey), _initVal);
        decltype(auto) x = meta::invokeReduce(_func, curKey, curVal, it->second);
        if (&x != &it->second) {
          it->second = x;
        }
      }
    } else {
      if (it != std::end(_index)) {
        it->second = meta::invokeReduce(_func, curKey, curVal, it->second);
      } else {
        it = _index.emplace_hint(
            it, ktype(curKey),
            meta::invokeReduce(_func, curKey, curVal, 
              (_initVal) ));
      }
    }
    if (_scan) {
      callKey<FO>(it);
      return;
    }
    if (_ordered) { 
      if (_first) {
        _first = false;
        _preIt = it;
      } else if (_preIt->first != curKey) {
        callKey<FO>(_preIt);
        _index.erase(_preIt);
        _preIt = it;
      }
    }
  }
private:
  virtual void _dataEnd(int) final override {
    if(!_scan) callEm<FO>();
    _first = true;
    _index.clear();
  }

  template <class T>
  auto callEm(typename std::enable_if<meta::isVector<T>{}>::type* dummy = 0) {
    std::vector<otype> res;
    res.reserve(_index.size());
    for (const auto& it : _index) {
      for (const auto& jt : it.second) {
        res.emplace_back(meta::slctTupleRef(meta::tieTup(it.first, jt) , Oslct{}));
      }
    }
    for (auto &jt : Link<itype, otype>::next()) {
      jt.second->dataEvent(res);
    }
  }

  template <class T>
  auto callEm(typename std::enable_if<!meta::isVector<T>{}>::type* dummy = 0) {
    std::vector<otype> res;
    res.reserve(_index.size());
    for (const auto& it : _index) {
      res.push_back(
          meta::slctTupleRef(meta::tieTup(it.first, it.second) , Oslct{})
      );
    }
    for (auto &jt : Link<itype, otype>::next()) {
      jt.second->dataEvent(res);
    }
  }

  template <class T>
  auto callKey(typename maptype::iterator it, typename std::enable_if<!meta::isVector<T>{}>::type* dummy = 0) {
    auto x = meta::slctTupleRef(meta::tieTup(it->first, it->second), Oslct{});
    for (auto &jt : Link<itype, otype>::next()) {
      jt.second->dataEvent(x);
    }
  }

  template <class T>
  auto callKey(typename maptype::iterator it, typename std::enable_if<meta::isVector<T>{}>::type* dummy = 0) {
    std::vector<otype> res;
    res.reserve(it->second.size());
    for(const auto& jt : it->second) {
      res.push_back(meta::slctTupleRef(meta::tieTup(it->first, jt), Oslct{}));
    }
    for (auto &jt : Link<itype, otype>::next()) {
      jt.second->dataEvent(res);
    }
  }
private:
  F _func;
  // const when UDF return by value so that non-const ref. argument
  // is a compile time error.
  std::conditional_t<TypeInfo::isRefRes, FO, const FO> _initVal;
  bool _scan{false};
  const bool _ordered{false};
  maptype _index;
  HashScheme _hash;
  EqWrapper _eq;
  bool _first{true};
  typename maptype::iterator _preIt;
};
}
} // namespace ezl ezl::detail

#endif // !REDUCE_EZL_H
