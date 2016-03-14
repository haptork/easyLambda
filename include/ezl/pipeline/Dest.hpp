/*!
 * @file
 * Abstract class `ezl::detail::Dest<T>`
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef __DEST_EZL_H__
#define __DEST_EZL_H__

#include <map>
#include <memory>
#include <vector>

#include "ezl/helper/Karta.hpp"

namespace ezl {
namespace detail {

template <typename T> class Source;
class Par;

/*!
 * @ingroup base
 * Defines basic functionality and data flow for the recieving end of a
 * pipeline.
 * The classes that are derived only from Destination are a dead end of the
 * pipeline while the classes that also inherit from source are a link
 * along with a source that passes the data to the next destination unit.
 *
 * It enables the traversal and data flow in forward direction when in a link.
 *
 * The Type defines the `Source<T>` that it can link to.
 * */
template <typename Type> class Dest {

public:
  Dest() : _id{Karta::inst().getId()} {}

  virtual void dataEvent(const Type &data) = 0;

  virtual void dataEvent(const std::vector<Type> &vData) {
    for (auto &it : vData) {
      dataEvent(it);
    }
  };

  /*!
   * Signals traversing forward without data such as end of data.
   * */
  virtual void signalEvent(int i) = 0;

  /*!
   * `Par` object recieved from the prior task unit.
   * */
  virtual void forwardPar(const Par *pr) = 0;

  /*!
   * to traverse forward in a pipeline and
   * return all the tasks.
   * */
  virtual std::vector<Task *> forwardTasks() = 0;

  /*!
   * While adding the prev. source, it also calls its
   * `next` with self. Use of self obviates the
   * need of enable_shared_from_this which looks cumbersome.
   *
   * @precondition: should be called with a shared_ptr object and
   *                that shapred_ptr should be passed as self.
   * */
  void prev(std::shared_ptr<Source<Type>> pr,
            std::shared_ptr<Dest<Type>> self) {
    if (!pr) return;
    assert(self.get() == this && "the self shared object needs to be passed");
    auto id = pr->id();
    if (_prev.find(id) == std::end(_prev)) {
      _prev[id] = pr;
      pr->next(self, pr);
    }
  }

  const int id() { return _id; }

  inline const auto &prev() const { return _prev; }

private:
  std::map<int, std::shared_ptr<Source<Type>>> _prev;
  int _id;
};
}
} // namespace ezl ezl::detail

#endif // !__DEST_EZL_H__
