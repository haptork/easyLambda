/*!
 * @file
 * Abstract class `ezl::detail::Root`
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef ROOT_EZL_H
#define ROOT_EZL_H

#include <ezl/pipeline/Source.hpp>
#include <ezl/pipeline/Dest.hpp>
#include <ezl/pipeline/Task.hpp>
#include <ezl/helper/Karta.hpp>
#include <ezl/helper/Par.hpp>

namespace ezl {

/*!
 * @ingroup base
 * The abstract class defines general behaviour for root sources in a pipeline.
 *
 * `pullData` pure virtual member for implementation in concrete classes.
 * */
template <typename Type> class Root : public Source<Type>, public Task {

public:
  Root(ProcReq p) : Task{p} {}

  virtual std::vector<Task *> root() override final {
    return std::vector<Task *>{this};
  }

  virtual std::vector<Task *> branchTasks() override final {
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

  /*!
   * Defining default behaviour for pulling
   *
   * Implementation for pullData in concrete classes.
   * */
  virtual void pull() override final {
    if (this->next().empty()) {
      return;
    }
    for (auto &it : this->next()) {  // pass task parallel info to the pipeline.
      it.second->forwardPar(&this->par());
    }
    if (this->par().inRange()) {
      _pullData();
    }
    for (auto &it : this->next()) {
      it.second->signalEvent(0);
    }
  }

private:
  virtual void _pullData() = 0;
  bool _traversingTasks{false};
};
} // namespace ezl

#endif // !ROOT_EZL_H
