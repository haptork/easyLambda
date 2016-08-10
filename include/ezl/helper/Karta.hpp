/*!
 * @file
 * class Karta
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef KARTA_EZL_H
#define KARTA_EZL_H

#include <vector>
#include <map>
#include <set>
#include <unordered_set>

#include <boost/mpi.hpp>

#include <ezl/pipeline/Task.hpp>
#include <ezl/helper/Par.hpp>
#include <ezl/helper/ProcReq.hpp>

namespace ezl {

enum class LogMode : int {
  none = 0x00,
  info = 0x01,
  warning = 0x02,
  error = 0x04,
  all = 0x07
};

using T = std::underlying_type_t <LogMode>;

inline LogMode operator | (LogMode lhs, LogMode rhs) {
  return (LogMode)(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline auto operator & (LogMode lhs, LogMode rhs) {
  return bool(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline LogMode& operator |= (LogMode& lhs, LogMode rhs) {
  lhs = (LogMode)(static_cast<T>(lhs) | static_cast<T>(rhs));
  return lhs;
}

namespace internal {
// ref:
// http://stackoverflow.com/questions/1453333/how-to-make-elements-of-vector-unique-remove-non-adjacent-duplicates
template <class FwdIterator>
inline FwdIterator stableUnique(FwdIterator first, FwdIterator last) {
  FwdIterator result = first;
  std::unordered_set<typename FwdIterator::value_type> seen;
  for (; first != last; ++first)
    if (seen.insert(*first).second) *result++ = *first;
  return result;
}

} // namespace internal

template <class T> class Source;
/*!
 * @ingroup helper
 * Singleton manager for managing ids, tags and other global properties. It
 * allocates processes to different tasks based on the request objects
 * (`ProcReq`).
 *
 * */
class Karta {

private:
  Karta() {
    _nProc = _comm.size();
    _rank = _comm.rank();
    refresh();
  }

public:
  static constexpr auto prllRatio = 0.50;

  static Karta &inst() {
    static Karta inst;
    return inst;
  };

  auto getId() { return _counter++; }

  const int nProc() { return _nProc; }

  void refresh() { 
    _procs.clear();
    _procs.reserve(_nProc);
    for (int i = 0; i < _nProc; i++) {
      _procs.push_back(make_pair(std::array<int, 2>{{0, 0}}, i));
    }
  }

  // A rudimentary log to be extended later.
  void log(std::string msg, LogMode mode = LogMode::info) {
    if (mode & _logMode) std::cerr<<_rank<<": "<<msg<<std::endl;
  }

  void log0(std::string msg, LogMode mode = LogMode::info) {
    if ((mode & _logMode) && _rank == 0) std::cerr<<msg<<std::endl;
  }

  void print(std::string str) {
    std::cout<<_rank<<": "<<str<<std::endl;
  }

  void print0(std::string str) {
    if (_rank == 0) std::cout<<str<<std::endl;
  }

  void logMode(LogMode mode) {
    _logMode = mode;
  }

  void runLocal(std::vector<Task*>& roots) {
    std::vector<int> proc {_rank};
    auto toDel = internal::stableUnique(std::begin(roots), std::end(roots));
    roots.erase(toDel, std::end(roots));
    std::vector<int> curProcs {_rank};
    Par par{curProcs, std::array<int, 3>{}, _rank};
    for (auto &it : roots) {
      it->par(par);
      auto temp = it->branchTasks();
      for (auto &jt : temp) jt->par(par);
    }
    for (auto &it : roots) it->prePull();
    for (auto &it : roots) it->pull();
  }

  template <class T>
  void run(Source<T> *obj, ProcReq p = ProcReq{}) {
    auto roots = obj->root();
    if (roots.empty()) return;
    if (p.type() == ProcReq::Type::local ||
        (_isRunning && p.type() == ProcReq::Type::none)) {
      runLocal(roots);
      return;
    }
    std::vector<int> curRun;
    std::vector<int> all;
    for (auto it : _procs) all.push_back(it.second);
    ++_isRunning;
    switch (p.type()) {
      case ProcReq::Type::count:
        curRun = _giveProcs(p.count(), all);
        break;
      case ProcReq::Type::ratio:
        curRun = _giveProcs(int(all.size() * p.ratio()), all);
        break;
      case ProcReq::Type::ranks:
        curRun = _giveProcs(p.ranks(), all);
        break;
      default: // none
        curRun = all;
        break;
    }
    auto toDel = internal::stableUnique(std::begin(roots), std::end(roots));
    roots.erase(toDel, std::end(roots));
    std::vector<std::vector<Task *>> bridges(roots.size());
    {
      std::set<Task *> prods;
      auto i = 0;
      for (auto &it : roots) {
        auto temp = it->branchTasks();
        for (auto &it : temp) {
          if (prods.find(it) == std::end(prods)) {
            bridges[i].push_back(it);
            prods.insert(it);
          }
        }
        i++;
      }
    }
    auto assigned = _assign(std::vector<std::vector<Task *>>{{roots}}, curRun,
                            std::vector<std::vector<int>>{{}});
    auto temp = _assign(bridges, curRun, assigned);
    for (auto &it : roots) it->prePull();
    for (auto &it : roots) it->pull();
    for (auto &it : _procs) {
      it.first[1] += it.first[2];
      it.first[0] = 0;
    }
    std::sort(std::begin(_procs), std::end(_procs));
    --_isRunning;
  }

  const int rank() const { return _rank; }

  const boost::mpi::communicator &comm() const { return _comm; }

private:
  // increments allocation count and sorts to get least occupied processs at
  // the top.
  void _markAlloc(std::vector<int> n) {
    auto updated = false;
    for (auto jt : n) {
      for (auto &it : _procs) {
        if (it.second == jt) {
          it.first[0]++;
          updated = true;
          break;
        }
      }
    }
    if (updated) std::sort(std::begin(_procs), std::end(_procs));
  }

  std::vector<int> _giveProcs(int count, const std::vector<int> &all) const {
    std::vector<int> cur;
    if (count < 0) count = all.size() - count; 
    if (count <= 0) count = 1;
    for (auto &it : all) {
      if (int(cur.size()) >= count) break;
      if (std::find(std::begin(cur), std::end(cur), it) == std::end(cur)) {
        cur.push_back(it);
      }
    }
    return cur;
  }

  template <typename C>
  std::vector<int> _giveProcs(const C &n, const std::vector<int> &all) const {
    std::vector<int> cur;
    for (auto it : n) {
      if (std::find(std::begin(all), std::end(all), it) != std::end(all)) {
        cur.push_back(it);
      }
    }
    if (cur.empty()) {
      std::cerr << "Process allocation to some units is not possible\
 with requested ranks. Please check the process ranks requested or leave it\
 for auto-allocation"
                << std::endl;
      return _giveProcs(1, all);
    }
    return cur;
  }

  /*!
   * @param prods producers, each row has same set of priority process ranks
   * @param curRun the set of process ranks that can be alloted
   * @param priority a set of ranks for each row of processes that are assigned
   *                 first irrespective of their present allocations.
   * @return vector of process ranks for each producer
   *
   * The rationale is task parallelism is good only if operations are heavy
   * else the new task should be fist assigned to processes that already have
   * the data. For e.g. data from four processes may be reduced in two
   * processes.  If the reduction is trivial/fast operation then it is better to
   * not communicate data from four processes to two new processes, instead two
   * of the four processes can do the reduction as well. Giving the reduction to
   * all four processes may again incur a lot of communication. So, priority has
   * the process ranks that are carrying the data, already. User may explicitely
   * call for task parallelism in which case the priority is solely on the basis
   * of how much free a process is.
   */
  std::vector<std::vector<int>>
  _assign(std::vector<std::vector<Task *>> prods, std::vector<int> curRun,
          std::vector<std::vector<int>> priority) {
    //assert(priority.size() == prods.size());
    std::vector<int> all;
    std::vector<std::vector<int>> assigned;
    for (auto i = 0; i < int(prods.size()); i++) {
      for (auto &jt : prods[i]) {
        all.clear();
        if (!jt->procReq().task()) {
          all.insert(std::begin(all), std::begin(priority[i]),
                     std::end(priority[i]));
        }
        for (auto it : _procs) { // push most free first
          if (std::find(std::begin(all), std::end(all), it.second) ==
                  std::end(all) &&
              std::find(std::begin(curRun), std::end(curRun), it.second) !=
                  std::end(curRun)) {
            all.push_back(it.second);
          }
        }
        std::vector<int> curProcs;
        switch (jt->procReq().type()) {
        case ProcReq::Type::count:
          curProcs = _giveProcs(jt->procReq().count(), all);
          break;
        case ProcReq::Type::ratio:
          if (priority[i].empty() || jt->procReq().task()) {
            curProcs = _giveProcs(int(all.size() * jt->procReq().ratio()), all);
          } else {
            curProcs =
                _giveProcs(int(priority[i].size() * jt->procReq().ratio()), all);
          }
          break;
        case ProcReq::Type::ranks:
          curProcs = _giveProcs(jt->procReq().ranks(), all);
          break;
        default: // none tag
          if (priority[i].empty()) {
            curProcs = all;
          } else if (jt->procReq().task()) {
            curProcs = _giveProcs(int(priority[i].size()), all);
          } else {
            auto share = int(priority[i].size() * prllRatio);
            curProcs = _giveProcs(share, all);
          }
          break;
        }
        if (_comm.rank() == 0 && (_logMode & LogMode::info)) {
          std::string s = "assigned process count: " +
                     std::to_string(curProcs.size()) + " viz.- ";
          for (auto it : curProcs) {
            s += std::to_string(it) + " ";
          }
          log(s);
        }
        jt->par(Par{curProcs,
                    std::array<int, 3>{{_curTag, _curTag + 1, _curTag + 2}},
                    _comm.rank()});
        _curTag += 3;
        // TODO: why resetting _curTag crashes some applications like displaced
        //if (_curTag + 3 > boost::mpi::environment::max_tag()) _curTag = 1;
        assigned.push_back(curProcs);
        _markAlloc(curProcs);
      }
    }
    return assigned;
  }

  boost::mpi::communicator _comm;
   int _curTag{1}; // Getting exhausted too quickly :(
  int _nProc;
  int _rank;
  std::vector<std::pair<std::array<int, 2>, int>> _procs;
  size_t _counter{1};
  std::vector<Task *> _curRoots;
  std::vector<std::vector<Task *>> _curOthers;
  LogMode _logMode{LogMode::error | LogMode::warning};
  int _isRunning {0};
  //int _curRootIndex;
};
} // namespace ezl 

#endif  // !KARTA_EZL_H
