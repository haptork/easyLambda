/*!
 * @file
 * class ProcReq
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef PROCREQ_EZL_H
#define PROCREQ_EZL_H

#include <vector>

namespace ezl {
enum class llmode : int {
  none = 0x00,
  task = 0x01,
  all = 0x02,
  shard = 0x04
};

using T = std::underlying_type_t <llmode>;

inline llmode operator | (llmode lhs, llmode rhs) {
  return (llmode)(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline auto operator & (llmode lhs, llmode rhs) {
  return bool(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline llmode& operator |= (llmode& lhs, llmode rhs) {
  lhs = (llmode)(static_cast<T>(lhs) | static_cast<T>(rhs));
  return lhs;
}

namespace detail {

/*!
 * @ingroup helper
 * Process request formed according to the user arguments for a task.
 * Karta allocates processes to tasks based on the ProcReq.
 * */
class ProcReq {

public:
  enum class Type { none, count, ratio, ranks };

  ProcReq() {}

  const Type type() const { return _type; }

  const bool task() const { return _task; }

  void setTask(bool task = true) { _task = task; }
  
  void resize(size_t n) { 
    if(_type == Type::ranks) {
      if (_ranks.size() > n) {
        _ranks.resize(n);  
      }
    } else {
      _type = Type::count;
      _count = n;
    }
  }

  ProcReq(int n) {
    _count = n;
    _type = Type::count;
  }

  ProcReq(float n) {
    _ratio = n;
    _type = Type::ratio;
  }

  ProcReq(double n) {
    _ratio = n;
    _type = Type::ratio;
  }

  ProcReq(std::vector<int> n) {
    _ranks = n;
    _type = Type::ranks;
  }

  const auto &count() const { return _count; }
  const auto &ratio() const { return _ratio; }
  const auto &ranks() const { return _ranks; }

private:
  bool _task{false};
  Type _type{Type::none};
  // TODO: unrestricted union
  int _count{0};
  float _ratio{0.};
  std::vector<int> _ranks{};
};
}
} // namespace ezl // namespace detail

#endif // !PROCREQ_EZL_H
