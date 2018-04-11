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

#include <boost/mpi.hpp>

#include <array>
#include <vector>

namespace ezl {

/*!
 * @ingroup helper
 * Parallel information class for tasks for communication.
 * The `Par` instance is allocated to a `Task` by Karta based on `ProcReq`
 * process requests made and processes available.
 * */
class Par {
public:
  Par(std::vector<int> procs, std::array<int, 3> t, int rank)
      : _rank{rank}, _ranks(procs), _tags{{t[0], t[1], t[2]}}, _isLocal{false} {
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

  Par() : _rank{_comm.rank()}, _ranks {_rank}, _isLocal{true} {}

  const bool &inRange() const { return _inRange; }

  const int &nProc() const { return _nProc; }
  const std::array<int, 3> &tags() const { return _tags; }
  const int &tag(int i) const { return _tags[i]; }
  const int &rank() const { return _rank; }
  const int &pos() const { return _pos; }

  const std::vector<int> &procAll() const { return _ranks; }

  auto begin() const { return std::cbegin(_ranks); }

  auto end() const { return std::cend(_ranks); }

  auto add(int p) {
    if (std::find(std::begin(_ranks), std::end(_ranks), p) == std::end(_ranks)) {
      _ranks.push_back(p);
      ++_nProc;
    }
    if (inRange() == false && rank() == p) {
      _pos = _ranks.size() - 1;
      _inRange = true;
    }
  }

  const auto &operator[](int index) const { return _ranks[index]; }

private:
  boost::mpi::communicator _comm;
  int _rank;
  std::vector<int> _ranks;
  int _nProc{1};
  std::array<int, 3> _tags{};
  int _pos{0};
  bool _inRange{true};
  bool _isLocal;
};
} // namespace ezl

#endif // !PAR_EZL_H
