/*!
 * @file
 * Abstract class `ezl::Link<T>` for in-between links in pipeline.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef __LINK_EZL_H__
#define __LINK_EZL_H__

#include <vector>

#include "ezl/pipeline/Dest.hpp"
#include "ezl/pipeline/Source.hpp"

namespace ezl {
namespace detail {

/*!
 * @ingroup base
 * Defines basic functionality and data flow for the links that are
 * destinations as well sources.
 *
 * pure virtual `dataEvent` and `dataEnd` for implementation by concrete classes
 * */
template <typename DestType, typename SourceType>
class Link : public Dest<DestType>, public Source<SourceType> {

public:
  /*!
   * Defining default behaviour for passing signal forward.
   * _visited helps to only signal once cyclic pipelines.
   * */
  virtual void signalEvent(int i) override final {
    if (_visited) return;
    _visited = true;
    _dataEnd(i);
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
    for (auto &it : this->prev()) {
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

  /*!
   * implementation for passing parallel information forward before pipeline
   * execution.
   * _visited helps to only signal once in cyclic pipelines.
   * */
  virtual void forwardPar(const Par *pr) override final {
    if (_visited) return;
    _visited = true;
    if (pr && !this->next().empty()) {
      for (auto &it : this->next()) {
        it.second->forwardPar(pr);
      }
    }
    _visited = false;
  }

private:
  virtual void _dataEnd(int) {} ;
  bool _traversingRoots{false};
  bool _traversingTasks{false};
  bool _visited{false};
};
}
} // namespace ezl ezl::detail

#endif // !__LINK_EZL_H__
