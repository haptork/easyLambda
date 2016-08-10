/*!
 * @file
 * Interface `ezl::detail::Task`
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef TASK_EZL_H
#define TASK_EZL_H

#include <vector>

#include <ezl/helper/Par.hpp>
#include <ezl/helper/ProcReq.hpp>

namespace ezl {

/*!
 * @ingroup base
 * The producer interface without type info.
 * */
class Task {
public:
  Task (ProcReq req) : _procReq{req} {}
  virtual void pull() = 0;
  virtual void prePull() {};
  virtual std::vector<Task *> branchTasks() = 0;
  void par(const Par &obj) { _par = obj; }
  const ProcReq &procReq() const { return _procReq; }

protected:
  inline const Par& par() const { return _par; }

private:
  Par _par;
  ProcReq _procReq;

};
} // namespace ezl

#endif // !TASK_EZL_H
