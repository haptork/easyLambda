/*!
 * @file
 * class DumpFile, unit for dumping to a file.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef CTORTELLER_EZL_TEST_H
#define CTORTELLER_EZL_TEST_H

#include <iostream>
#include <string>

struct ctorTeller {

  ctorTeller(std::string s = "", bool isReset = false, bool verbose = true): _name{s}, _verbose{verbose} {
    if (_verbose) std::cout<<"constructing: "<<_name<<std::endl;
    if (isReset) reset();
    ++_ctor;
  }

  ctorTeller(const ctorTeller& obj) {
    _name = obj._name;
    _verbose = obj._verbose;
    if (_verbose) std::cout<<"copying: "<<_name<<std::endl;
    ++_copyCtor;
  }

  ctorTeller(const ctorTeller&& obj) {
    _name = obj._name;
    _verbose = obj._verbose;
    if (_verbose) std::cout<<"moving: "<<_name<<std::endl;
    ++_moveCtor;
  }

  auto operator = (const ctorTeller&& obj) {
    _name = obj._name;
    _verbose = obj._verbose;
    if (_verbose) std::cout<<"moveAssigning: "<<_name<<std::endl;
    ++_moveAssign;
  }

  auto operator = (const ctorTeller& obj) {
    _name = obj._name;
    _verbose = obj._verbose;
    if (_verbose) std::cout<<"copyAssigning: "<<_name<<std::endl;
    ++_copyAssign;
  }

  auto operator == (const ctorTeller& obj) const {
    if (_verbose) display("teller==");
    return _name == obj._name;
  }

  const auto& name() const { return _name; }

  static void reset() {
    _ctor = 0;
    _copyCtor = 0;
    _moveCtor = 0;
    _copyAssign = 0;
    _moveAssign = 0;
  }

  static void display(std::string tag = "") {
    std::cout << "- ctor: " << _ctor << "; copy: " << _copyCtor
              << "; move: " << _moveCtor << "; copyAssign: " << _copyAssign
              << "; moveAssign: " << _moveAssign<< " -" << tag << std::endl;
  };

  static const int& ctor() { return _ctor; }
  static const int& copyctor() { return _copyCtor; }
  static const int& movector() { return _moveCtor; }
  static const int& copyassign() { return _copyAssign; }
  static const int& moveassign() { return _moveAssign; }

  int val {0};
private:
  std::string _name;
  bool _verbose;
  static int _ctor;
  static int _copyCtor;
  static int _moveCtor;
  static int _copyAssign;
  static int _moveAssign;
};

#endif // !CTORTELLER_EZL_TEST_H
