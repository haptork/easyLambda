/*!
 * @file
 * class Par
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef PAR_EZL_H
#define PAR_EZL_H

#include <array>
#include <vector>

namespace ezl {
namespace detail {

/*!
 * @ingroup helper
 * Parallel information class for tasks for communication.
 * The `Par` instance is allocated to a `Task` by Karta based on `ProcReq`
 * process requests made and processes available.
 * */
class Par {
public:
  Par(std::vector<int> procs, std::array<int, 3> t, int rank)
      : _ranks(procs), _tags{{t[0], t[1], t[2]}}, _rank{rank} {
    _nProc = procs.size();
    auto it = std::find(std::begin(procs), std::end(procs), _rank);
    if (it == std::end(procs)) {
      _pos = -1;
      _inRange = false;
    } else {
      _pos = it - std::begin(procs);
      _inRange = true;
    }
  }

  Par() : _nProc{0}, _pos{-1}, _inRange{false}, _rank{-1} {}

  const bool &inRange() const { return _inRange; }

  const int &nProc() const { return _nProc; }
  const std::array<int, 3> &tags() const { return _tags; }
  const int &tag(int i) const { return _tags[i]; }
  const int &rank() const { return _rank; }
  const int &pos() const { return _pos; }

  const std::vector<int> &procAll() const { return _ranks; }

  auto begin() const { return std::cbegin(_ranks); }

  auto end() const { return std::cend(_ranks); }

  const auto &operator[](int index) const { return _ranks[index]; }

private:
  std::vector<int> _ranks;
  int _nProc;
  std::array<int, 3> _tags;
  int _pos;
  bool _inRange;
  int _rank;
};
}
} // namespace ezl ezl::detail

#endif // !PAR_EZL_H
