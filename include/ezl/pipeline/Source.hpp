/*!
 * @file
 * Abstract class `ezl::detail::Source<T>`
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef SOURCE_EZL_H
#define SOURCE_EZL_H

#include <map>
#include <memory>
#include <vector>

#include "ezl/helper/Karta.hpp"

namespace ezl {
namespace detail {

template <typename T> class Dest;
class Task;

/*!
 * @ingroup base
 * Defines basic functionality and data flow for the source end of a pipeline.
 * A source can be a `Root` producer, `Link` or a `Bridge`.
 * It traversal and data flow is in backward direction from its destination.
 *
 * The Type defines the `Dest<T>` it can link to.
 * */
template <typename Type> class Source {
public:
  using otype = Type;

  Source() : _id{Karta::inst().getId()} {}

  /*!
   * While adding the next dest, it also calls
   * its `prev` with self. Use of self obviates the
   * need of enable_shared_from_this which looks cumbersome.
   *
   * @precondition: should be called with a shared_ptr object and
   *                that shapred_ptr should be passed as self.
   * */
  virtual void next(std::shared_ptr<Dest<Type>> nx,
                    std::shared_ptr<Source<Type>> self) {
    if (!nx) return;
    assert(self.get() == this && "the self shared object needs to be passed");
    auto id = nx->id();
    if (_next.find(id) == std::end(_next)) {
      _next[id] = nx;
      nx->prev(self, nx);
    }
  }

  /*!
   * traverse back from any unit to all the connected root sources.
   *
   * A link traverses up while a root returs itself (base case).
   * */
  virtual std::vector<Task *> root() = 0;

  const int id() { return _id; }

  inline auto &next() const { return _next; }

private:
  std::map<int, std::shared_ptr<Dest<Type>>> _next;
  int _id;
};
}
} // namespace ezl ezl::detail

#endif // !SOURCE__EZL_H
