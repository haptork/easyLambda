/*!
 * @file
 * class Zip. For zipping / joining two rows by key or order of rows.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef ZIP_EZL_H
#define ZIP_EZL_H

#include <deque>
#include <tuple>

#include <boost/unordered_map.hpp>

#include <ezl/pipeline/Source.hpp>
#include <ezl/pipeline/Dest.hpp>
#include <ezl/helper/meta/slctTuple.hpp>

namespace ezl {
namespace detail {

/*!
 * @ingroup mapreduce
 * unit for zipping(functional zip) / joining rows from two sources of
 * different types based on key columns or order of rows. 
 *
 * See examples for using with builders or unittests for direct use.
 *
 * */
template <class I1, class I2, class Kslct1, class Kslct2, class Oslct,
         class HashScheme>
struct Zip
    : public virtual Dest<I1>,
      public virtual Dest<I2>,
      public Source<typename meta::SlctTupleRefType<meta::TupleTieType<I1, I2>,
                                                    Oslct>::type> {
private:
  struct EqWrapper {
  public: 
    template <class T, class U>
    bool operator () (const T& a, const U& b) const { return a == b; }
  };
public:
  using otype =
      typename meta::SlctTupleRefType<meta::TupleTieType<I1, I2>, Oslct>::type;
  using ktype = typename meta::SlctTupleType<I1, Kslct1>::type;
  using v1type = std::deque<typename meta::SlctTupleType<I1>::type>;
  using v2type = std::deque<typename meta::SlctTupleType<I2>::type>;
  using maptype1 =  boost::unordered_map<ktype, v1type, HashScheme>;
  using maptype2 =  boost::unordered_map<ktype, v2type, HashScheme>;

  using Dest<I1>::dataEvent;
  using Dest<I2>::dataEvent;

  static constexpr int osize = std::tuple_size<otype>::value;

  Zip() = default;

  virtual void dataEvent(const I1 &data) final override {
    auto curKey = meta::slctTupleRef(data, Kslct1{});
    auto it = _index1.find(curKey, _hash, _eq);
    if (it != std::end(_index1)) {
      it->second.push_back(data); 
    } else {
      v1type v {data};
      it = _index1.emplace_hint(it, ktype(curKey), v); 
    } 
    auto it2 = _index2.find(curKey, _hash, _eq);
    if (it2 != std::end(_index2)) _flush(it, it2);
  }

  virtual void dataEvent(const I2 &data) final override {
    auto curKey = meta::slctTupleRef(data, Kslct2{});
    auto it = _index2.find(curKey, _hash, _eq);
    if (it != std::end(_index2)) {
      it->second.push_back(data); 
    } else {
      v2type v {data};
      it = _index2.emplace_hint(it, ktype(curKey), v); 
    } 
    auto it1 = _index1.find(curKey, _hash, _eq);
    if (it1 != std::end(_index1)) _flush(it1, it);
    /*
    std::cout<<"index2\n";
    for (auto it = _index2.begin(); it != _index2.end(); ++it) {
      std::cout<<it->first<<std::endl;
    }
    */
  }

  /*!
   * implementation for passing parallel information forward before pipeline
   * execution.
   * _visited helps to only signal once in cyclic pipelines.
   * */
  virtual void forwardPar(const Par *pr) override final {
    if (_visited) return;
    _visited = true;
    Dest<I1>::incSig();
    if (pr && !this->next().empty()) {
      for (auto &it : this->next()) {
        it.second->forwardPar(pr);
      }
    }
    _visited = false;
  }

  /*!
   * Defining default behaviour for passing signal forward.
   * _visited helps to only signal once cyclic pipelines.
   * */
  virtual void signalEvent(int i) override final {
    if (_visited) return;
    _visited = true;
    if (Dest<I1>::decSig() == 0) _dataEnd(i);
    if (!this->next().empty()) {
      for (auto &it : this->next()) {
        it.second->signalEvent(i);
      }
    }
    _visited = false;
  }

  /*!
   * implementation for traversing back to root in a pipeline.
   * */
  virtual std::vector<Task *> root() override final {
    std::vector<Task *> roots;
    if (_traversingRoots) return roots;
    _traversingRoots = true;
    for (auto &it : Dest<I1>::prev()) {
      auto temp = it.second->root();
      roots.insert(std::begin(roots), std::begin(temp), std::end(temp));
    }
    for (auto &it : Dest<I2>::prev()) {
      auto temp = it.second->root();
      roots.insert(std::begin(roots), std::begin(temp), std::end(temp));
    }
    _traversingRoots = false;
    return roots;
  }

  /*!
   * implementation for returning all the tasks in the pipeline.
   * */
  virtual std::vector<Task *> forwardTasks() override final {
    std::vector<Task *> tasks;
    if (_traversingTasks) return tasks;
    _traversingTasks = true;
    for (auto &it : this->next()) {
      auto temp = it.second->forwardTasks();
      tasks.insert(std::end(tasks), std::begin(temp), std::end(temp));
    }
    _traversingTasks = false;
    return tasks;
  }

  bool prev(std::shared_ptr<Source<I1>> pr,
            std::shared_ptr<Dest<I1>> self) override final {
    return Dest<I1>::prev(pr, self);
  }

  bool prev(std::shared_ptr<Source<I2>> pr,
            std::shared_ptr<Dest<I2>> self) override final {
    return Dest<I2>::prev(pr, self);
  }
 
  virtual void unPrev(Source<I1>* pr) override final { Dest<I1>::unPrev(pr); }
 
  virtual void unPrev(Source<I2>* pr) override final { Dest<I2>::unPrev(pr); }

  virtual void unPrev() override final {
    Dest<I1>::unPrev();
    Dest<I2>::unPrev();
  }

  void unLink() {
    unPrev();
    Source<otype>::unNext();
  }

private:
  void _dataEnd(int) {
    for (auto it = _index1.begin(); it != _index1.end();) {
      auto it2 = _index2.find(it->first, _hash, _eq);
      if (it2 != std::end(_index2)) {
        _flush(it, it2);
        _index2.erase(it2);
      }
      it = _index1.erase(it);
    }
    //auto it1 = _index1.find(curKey, _hash, _eq);
    _index1.clear();
    _index2.clear();
  }

  void _flush(const typename maptype1::iterator &it1,
              const typename maptype2::iterator &it2) {
    auto len = std::min(it1->second.size(), it2->second.size());
    if (len == 0) return;
    for (size_t i = 0; i < len; ++i) {
      auto otemp =
          meta::slctTupleRef(meta::tieTup(it1->second.front(),
                                          it2->second.front()),
                             Oslct{});
      it1->second.pop_front();
      it2->second.pop_front();
      for (auto &it : Source<otype>::next()) {
        it.second->dataEvent(otemp);
      }
    }
    if (it1->second.empty()) _index1.erase(it1);
    if (it2->second.empty()) _index2.erase(it2);
  }

  maptype1 _index1;
  maptype2 _index2;
  EqWrapper _eq;
  HashScheme _hash;
  bool _visited{false};
  bool _traversingRoots{false};
  bool _traversingTasks{false};
};
}
} // namespace ezl ezl::detail

#endif // !ZIP_EZL_H
