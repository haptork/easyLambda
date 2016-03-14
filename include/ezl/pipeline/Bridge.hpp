/*!
 * @file
 * abstract class Bridge
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef __BRIDGE_EZL_H__
#define __BRIDGE_EZL_H__

#include "ezl/pipeline/Source.hpp"
#include "ezl/pipeline/Task.hpp"
#include "ezl/helper/Karta.hpp"
#include "ezl/helper/Par.hpp"

namespace ezl {
namespace detail {

/*!
 * @ingroup base
 * Abstract class with general behaviour for units that make the pipeline
 * parallel.
 * */
template <typename IO>
class Bridge : public Dest<IO>, public Source<IO>, public Task {

public:
  Bridge(ProcReq p) : Task{p} {}

  /*!
   * traverse back to the source looking for the root.
   * */
  virtual std::vector<Task *> root() override final {
    std::vector<Task *> roots;
    if (_traversingRoots) return roots;
    _traversingRoots = true;
    for (auto &it : Dest<IO>::prev()) {
      auto temp = it.second->root();
      roots.insert(std::begin(roots), std::begin(temp), std::end(temp));
    }
    _traversingRoots = false;
    return roots;
  }

  virtual std::vector<Task *> forwardTasks() override final {
    std::vector<Task *> tasks;
    if (_traversingTasks) return tasks;
    tasks.push_back(this);
    _traversingTasks = true;
    for (auto &it : Source<IO>::next()) {
      auto temp = it.second->forwardTasks();
      tasks.insert(std::end(tasks), std::begin(temp), std::end(temp));
    }
    _traversingTasks = false;
    return tasks;
  }

  /*!
   * The next units in the pipeline get the parallel information assigned to
   * this task, while the prior parallel info is stored in a data member.
   * This is important because bridge sends data from units working in
   * processes of prior task to the processes assigned to it and
   * passes the data to next units.
   * _visited helps to only signal once in cyclic pipelines.
   *
   * */
  virtual void forwardPar(const Par *pr) override final {
    if (_visited) return;
    _visited = true;
    _parHandle = pr;
    _dataBegin();
    if (!Source<IO>::next().empty()) { // implies _par.inRange()
      for (auto &it : Source<IO>::next()) {
        it.second->forwardPar(&(Task::par()));
      }
    }
    _visited = false;
  }

  /*!
   * Defining default behaviour for passing signal forward.
   * _visited helps to only signal once in cyclic pipelines.
   * */
  virtual void signalEvent(int i) override final {
    if (_visited) return;
    _visited = true;
    _dataEnd(Task::par().nProc());
    if (!this->next().empty()) {
      for (auto &it : this->next()) {
        it.second->signalEvent(i);
      }
    }
    _visited = false;
  }

  virtual void pull() override final {}
  virtual std::vector<Task *> branchTasks() override final { return forwardTasks(); }

protected:
  inline const auto& parHandle() const { return _parHandle; }
private:
  virtual void _dataBegin() = 0;  // Implementation for dataBegin.
  virtual void _dataEnd(int j) = 0;  // Implementation for dataEnd.
  const Par* _parHandle;  // parallel info for prior task
  bool _traversingRoots{false};
  bool _traversingTasks{false};
  bool _visited{false};
};
}
} // namespace ezl ezl::detail

#endif // !__BRIDGE_EZL_H__
