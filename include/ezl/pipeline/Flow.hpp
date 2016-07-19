/*!
 * @file
 * class Flow for representing flows based on I/O of streams.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef FLOW_EZL_H
#define FLOW_EZL_H

#include <memory>
#include <vector>

#include <ezl/pipeline/Dest.hpp>
#include <ezl/pipeline/Source.hpp>

namespace ezl {

using namespace ezl::detail;

/*!
 * @ingroup base
 * An encapsulation that can represent a complete data-flow based on first and
 * last units. It can be seen as something to represent a black-box of
 * calculation that may contain many more units, including other Flow objects
 * as well but is represented by only its input and output types. It does not
 * itself become part of a data-flow but delegates prev and next connections to
 * its first and last units. 
 * */

template <class I, class O>
struct Flow : public Dest<I>, public Source<O> {
public:
  using itype = I;
  using otype = O;

  Flow(std::vector<std::shared_ptr<Dest<I>>> first,
       std::vector<std::shared_ptr<Source<O>>> last)
      : _first(first), _last(last) {}

  Flow() = default;

  template <class _>
  Flow(std::shared_ptr<Flow<I, _>> obj) : _first(obj->first()), _flprev(obj->flprev()) { }

  auto addFirst(const std::shared_ptr<Dest<I>>& dest) { 
    if (!dest) return;
    auto id = dest->id();
    if (_first.find(id) != std::end(_first)) return;
    _first[id] = dest;
    for (auto &it : _flprev) it.second->next(dest, it.second);  
 }

  auto addLast(const std::shared_ptr<Source<O>>& src) { 
    if (!src) return;
    auto id = src->id();
    if (_last.find(id) != std::end(_last)) return;
    _last.emplace(id, src);
    for (auto &it : _flnext) it.second->prev(src, it.second);  
  }

  const auto& first() { return _first; }

  const auto& last() { return _last; }

  const auto& flprev() { return _flprev; }

  auto isEmpty() { return (_first.size() == 0 && _last.size() == 0); }

  virtual bool next(std::shared_ptr<Dest<O>> nx,
                    std::shared_ptr<Source<O>> self) final override {
    flnext(nx);
    return false;
  }

  void flnext(std::shared_ptr<Dest<O>> nx) {
    if (!nx) return;
    for (auto &it : _last) it.second->next(nx, it.second);
    auto id = nx->id();
    if (_flnext.find(id) == std::end(_flnext)) {
      _flnext.emplace(id, nx);
    }
  }

  virtual void unNext(Dest<O>* nx) final override {
    if (!nx) return;
    for (auto &it : _last) it.second->unNext(nx);
    auto id = nx->id();
    if (_flnext.find(id) != std::end(_flnext)) {
      _flnext.erase(id);
    }
  }

  virtual void unNext() final override {
    for (auto& it : _flnext) {
      for (auto &jt : _last) jt.second->unNext(it.second.get());
    }
    _flnext.clear();
  }

  virtual bool prev(std::shared_ptr<Source<I>> pr,
                    std::shared_ptr<Dest<I>> self) final override {
    flprev(pr);
    return false;
  }

  void flprev(std::shared_ptr<Source<I>> pr) {
    if (!pr) return;
    for(auto &it : _first) it.second->prev(pr, it.second);
    auto id = pr->id();
    if (_flprev.find(id) == std::end(_flprev)) {
      _flprev.emplace(id, pr);
    }
  }

  virtual void unPrev(Source<I>* pr) final override {
    if (!pr) return;
    for (auto &it : _first) it.second->unPrev(pr);
    auto id = pr->id();
    if (_flprev.find(id) != std::end(_flprev)) {
      _flprev.erase(id);
    }
  }

  virtual void unPrev() final override {
    for (auto& it : _flprev) {
      for (auto &jt : _first) jt.second->unPrev(it.second.get());
    }
    _flprev.clear();
  }

  void unlink() {
    unPrev();
    unNext();
  }

  virtual void dataEvent(const itype &data) final override {}

  virtual void dataEvent(const std::vector<itype> &vData) final override {}

  /*!
   * implementation for passing parallel information forward before pipeline
   * execution.
   * Do not gets called usually as it is not connected to units
   * in the pipeline and hence not connected to any root.
   * */
  virtual void forwardPar(const Par *pr) override final {}

  virtual void signalEvent(int i) override final {}

  /*!
   * implementation for traversing back to root in a pipeline.
   * */
  virtual std::vector<Task *> root() override final {
    std::vector<Task *> roots;
    if (_traversingRoots) return roots;
    _traversingRoots = true;
    for (auto &it : this->_last) {
      auto temp = it.second->root();
      roots.insert(std::begin(roots), std::begin(temp), std::end(temp));
    }
    _traversingRoots = false;
    return roots;
  }

  /*!
   * implementation for returning all the tasks in the pipeline.
   * Do not gets called usually as it is not connected to units
   * in the pipeline and hence not connected to any root.
   * */
  virtual std::vector<Task *> forwardTasks() override final {
    return std::vector<Task *> {};
  }

private:
  bool _traversingRoots{false};
  bool _traversingTasks{false};
  bool _visited{false};
  std::map<int, std::shared_ptr<Dest<I>>> _first;
  std::map<int, std::shared_ptr<Source<O>>> _last;

  std::map<int, std::shared_ptr<Source<I>>> _flprev;
  std::map<int, std::shared_ptr<Dest<O>>> _flnext;
};
} // namespace ezl

#endif // ! FLOW_EZL_H
