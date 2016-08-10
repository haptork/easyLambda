/*!
 * @file
 * class DumpExpr
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef DUMPEXPR_EZL_H
#define DUMPEXPR_EZL_H

#include <ezl/helper/meta/slctTuple.hpp>
#include <ezl/units/Dump.hpp>

namespace ezl {
namespace detail {

/*!
 * @ingroup builder
 * For adding a `Dump` unit as a branch to the current unit. This
 * can be specified with property `dump` without finalizing build of
 * the current uni. The expression can be used by any unit builder
 * like `MapBuilder` `ReduceBuilder` etc.
 *
 * T is CRTP parent type.
 * */
template <class T, class O> struct DumpExpr {
protected:
  DumpExpr() = default;

  template <class I>
  auto _postBuild(I& obj) {
    if (_name != defStr) {
      auto dObj = std::make_shared<
          Dump<typename std::decay_t<decltype(obj)>::element_type::otype>>(_name, _header);
      obj->next(dObj, obj);
    }
  }

public:
  // get the output on standard output or file
  // @param name optional file name, defaults to stdout
  // @param header optional header string to show at the top
  auto& dump(std::string name = "", std::string header = "") {
    _name = name;
    _header = header;
    return ((T *)this)->_self();
  }
  
  // select output columns by index starting from 1 e.g. cols<3, 1>() selects
  // third and first column
  template <int... Os> auto cols() {
    return ((T *)this)->colsSlct(meta::slct<Os...>{});
  }

  // drop columns from output by index starting from 1 e.g. colsDrop<3, 1>()
  // drops third and first column and selects the rest.
  template <int... Ns>
  auto colsDrop() {
    using NO = typename meta::setDiff<O, meta::slct<Ns...>>::type;
    return ((T *)this)->colsSlct(NO{});
  }

  // get name and header for dump
  auto dumpProps() { return std::tie(_name, _header); }

  // set name and header for dump
  void dumpProps(std::tuple<const std::string&, const std::string&> props) {
    std::tie(_name, _header) = props;
  }

private:
  const std::string defStr{"__none__"};
  std::string _name{defStr};
  std::string _header{""};
};
}
} // namespace ezl ezl::detail

#endif // !DUMPEXPR_EZL_H
