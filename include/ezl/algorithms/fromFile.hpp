/*!
 * @file
 * class FromFile, unit for loading data from a file.
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef FROMFILE_EZL_H
#define FROMFILE_EZL_H

#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <ezl/helper/meta/lexCastTuple.hpp>
#include <ezl/helper/meta/slctTuple.hpp>
#include <ezl/helper/vglob.hpp>
#include <ezl/helper/Karta.hpp>

namespace ezl {
/*!
 * @ingroup mapreduce
 * Defines status messages for use with every row while multiple processes read
 * in parallel. Especially useful for reading normalized data in parallel(i.e.
 * where header to a group of rows contain information that is needed to process
 * various rows), or keeping a group of rows in a single process for efficient
 * reduction later etc.

 * The enum is not in FromFile since including in FromFile would mean that to
 * access the enum, one would need to know the template parameters as FromFile
 * is a template class.
 *
 * See examples for using with builders or unittests for direct use.
 *
 * */
enum class rs {
  br, // break
  nobr,
  prior, // prior line was breakable
  ignore, // skip if break occurs else execute
  eod,  // end of data
  eof,  // end of file
};

/*!
 * A function object to read lammps output files in parallel to be used with
 * FromFile. Lammps output has time-step at the top. The function object puts the
 * time-step in every row and makes sure that even if multiple processes read
 * a single file with various time-steps, a single time-step is read
 * by the same process with time-step value appended to each row. In a way
 * it denormalizes the text file. This can be used as an example to write
 * readers for other formats.
 *
 * Example usage:
 * @code
 * // It prepends integer timestep value to actual columns in the file
 * load<int, int, float, float, float>("*.xyz").parse(ezl::lammpsSchema).build();
 * @endcode
 * */
class lammpsSchema {
public:
  auto operator()(std::vector<std::string> &v) {
    if (v.empty()) {
      if (_in) {
        return std::make_pair(false, rs::br);
      } else {
        return std::make_pair(false, rs::br);
      }
    }
    if (_next) {
      try {
        _timeStep = std::stoi(v[0]);
      } catch (std::invalid_argument &err) {
        return std::make_pair(false, rs::br);
      }
      _next = false;
    }
    if (v.size() > 1 && v[1] == "TIMESTEP") {
      _next = true;
      _in = true;
      return std::make_pair(false, rs::prior);
    }
    if (_in) {
      v.push_back(std::to_string(_timeStep));
      return std::make_pair(true, rs::nobr);
    }
    return std::make_pair(false, rs::nobr);
  }
private:
  int _timeStep{0};
  bool _next{false};
  bool _in{false};
};

namespace detail {

template <class C>
inline void stripContainer(const C &inp, C &out, const std::vector<int> &mask) {
  int len = mask.size();
  for (int i = 0; i < len; i++) {
    if (mask[i] > 0 && mask[i] <= int(inp.size())) {
      out.push_back(inp[mask[i] - 1]);
    }
  }
}

template <class C>
inline void stripContainerBool(const C &in, C &out,
                               const std::vector<int> &mask) {
  int len = mask.size();
  for (int i = 0; i < len; i++) {
    if (mask[i] != 0) { out.push_back(in[i]); }
  }
}
} // namespace ezl::detail


struct FromFileProps {
  char rDelim{'\n'};
  std::string cDelims{" "};
  std::vector<int> cols;
  std::vector<std::string> headers;
  std::vector<int> drop;
  std::vector<std::string> dropHead;
  std::function<std::pair<bool, rs>(std::vector<std::string> &)> check;
  bool strict{true};
  bool tilleof{false};
  bool addFileName{false};
  std::vector<std::string> fnames;
  bool share{true};
  int rowsMax{0};
  std::string fpat = "";
  size_t filesMax{0};
};

/*!
 * @ingroup mapreduce
 * Root unit for loading data from file(s) in parallel with lots of options.
 *
 * */
template <class I, class Kslct>
class FromFile {
public:

  using otype = I;
  static constexpr auto osize = detail::meta::colCountImpl<0,I>::value;
  using ktype = typename detail::meta::SlctTupleType<otype, Kslct>::type;
  static constexpr int ksize = std::tuple_size<ktype>::value;

/* !
 * ctor has many params since it is meant to be built with builder expression or
 * property based builder
 *
 * @param fpat glob pattern for file(s) to be read.
 * @param pr process request object for the task.
 * @param rDelim row delimiter
 * @param cDelims column delimiter string
 * @param cols index of columns to be selected from each row.
 * @param ch can be used to add or edit columns or even denormalize on the go
             by returning appropriate rs enum value.
 * @param strict if true then the number of columns should match exactly for
                 the row to be accepted else null padding or splicing of rows
                 is performed to make it of same size as expected.
 * @param tillEOF parallel reading on per file basis rather than total size.
                processes start to read from beginning of file and read till
                end.
 * @param addFileName addFileName to every row or not.
 * @param share Does processes have same glob pattern/files that are to be
                shared among them.
 * */
  template <class K>
  FromFile(const FromFile<I, K>& obj) : _props(obj.props()) {}
 
  FromFile(std::string fpat) {
    _props.fpat = fpat;
  }

  FromFile(std::vector<std::string> fnames) {
    _props.fnames.insert(_props.fnames.begin(), fnames.begin(), fnames.end());
  }

  const auto& props() const { return _props; }

  auto rowSeparator(char c) {
    _props.rDelim = c;
    return std::move(*this);
  }

  auto colSeparator(std::string s) {
    _props.cDelims = s;
    return std::move(*this);
  }

  auto cols(std::initializer_list<int> fl) {
    if (!_props.cols.empty()) _props.cols.clear();
    if (!_props.headers.empty()) _props.headers.clear();
    _props.cols.insert(_props.cols.begin(), fl.begin(), fl.end());
    return std::move(*this);
  }

  auto cols(std::initializer_list<std::string> headerList) {
    if (!_props.headers.empty()) _props.headers.clear();
    if (!_props.cols.empty()) _props.cols.clear();
    _props.headers.insert(_props.headers.begin(), headerList.begin(), headerList.end());
    return std::move(*this);
  }

  auto dropCols(std::initializer_list<int> fl) {
    if (!_props.drop.empty()) _props.drop.clear();
    _props.drop.insert(_props.drop.begin(), fl.begin(), fl.end());
    return std::move(*this);
  }

  auto dropCols(std::initializer_list<std::string> dropHead) {
    if (!_props.drop.empty()) _props.drop.clear();
    if (!_props.dropHead.empty()) _props.dropHead.clear();
    _props.dropHead.insert(_props.dropHead.begin(), dropHead.begin(),
                           dropHead.end());
    return std::move(*this);
  }

  template <int N, int... Ns>
  auto ordered() {
    return FromFile<I, detail::meta::slct<N, Ns...>>(*this); 
  }

  auto parse(
      std::function<std::pair<bool, rs>(std::vector<std::string> &)> c) {
    _props.check = c;
    return std::move(*this);
  }

  auto noStrict(bool flag = true) {
    _props.strict = !flag;
    return std::move(*this);
  }

  auto tillEOF(bool eof = true) {
    _props.tilleof = eof;
    return std::move(*this);
  }

  auto maxFilesToRead(size_t count) {
    _props.filesMax = count;
    return std::move(*this);
  }

  auto addFileName(bool f = true) {
    _props.addFileName = f;
    return std::move(*this);
  }

  auto filePattern(std::string s) {
    _props.fpat = s;
    return std::move(*this);
  }

  auto noShare(bool isShare = false) {
    _props.share = isShare;
    return std::move(*this);
  }

  auto top(int nRows = 10) {
    _props.rowsMax = nRows;
    return std::move(*this);
  }
  
  auto lammps() {
    return parse(lammpsSchema());
  }

  inline auto operator() () {
    rs cur;
    while (true) {
      if (!loaded) {
        if (!_nextFile()) break;
        else loaded = true;
      }
      std::tie(cur, accept) = _lineHai();
      if (cur == rs::eof) {
        loaded = false;
      }
      if (accept) {
        return std::tie(_out, accept);
      }
    }
    loaded = false;
    _cur = -1;
    return std::tie(_out, loaded);
  }

  /*!
   * Divide all the files data equally between the task processes.
   * */
  void operator() (int pos, std::vector<int> procs) {
    in = preBreak = prepreBreak = false;
    first = true;
    _rowsRead = 0;
    if(!_props.headers.empty()) _headerCols(_props.cols, _props.headers);
    if(!_props.dropHead.empty()) _headerCols(_props.drop, _props.dropHead);
    _sanityCheck();
    if(!_props.fpat.empty()) {
      _props.fnames.clear();
      _props.fnames = vglob(_props.fpat, _props.filesMax);
      if(_props.fnames.empty()) {
        Karta::inst().log("No file found for pattern: "+_props.fpat, LogMode::warning);
        return;
      }
    }
    _pos = pos;
    if (pos == -1 || _props.fnames.empty()) return;
    if (!_props.share) {
      _props.tilleof = true;
      _rBeginFile = 0;
      _rEndFile = _props.fnames.size() - 1;  // not return
      return;
    }
    if (_props.tilleof) {
      _divideFiles(pos, procs);
      return;
    }
    auto total = 0LL;
    std::vector<long long> cumSizes;
    cumSizes.reserve(_props.fnames.size() + 1);
    for (auto &it : _props.fnames) {
      std::ifstream in(it, std::ifstream::ate | std::ifstream::binary);
      int len = in.tellg();
      cumSizes.push_back(total);
      total += len;
    }
    cumSizes.push_back(total);
    long long share = total / procs.size(); // TODO LL cast
    _rBeginFile = -1;
    _rBeginByte = 0;
    auto rTotalBeginByte = share * pos;
    for (auto it : cumSizes) {
      if (rTotalBeginByte < it) { break; }
      _rBeginFile++;
    }
    auto preSize = 0LL;
    preSize = cumSizes[_rBeginFile];
    _rBeginByte = rTotalBeginByte - preSize;
    if (pos == int(procs.size()) - 1) {
      _rEndFile = _props.fnames.size() - 1;
      _rEndByte = cumSizes[cumSizes.size() - 1];
      if (_rEndFile > 0) { _rEndByte -= cumSizes[cumSizes.size() - 2]; }
    } else {
      auto rTotalEndByte = share * (pos + 1);
      _rEndFile = 0;
      for (auto it : cumSizes) {
        if (rTotalEndByte < it) { break; }
        _rEndFile++;
      }
      if (_rEndFile > 0) {
        _rEndFile--;
        preSize = cumSizes[_rEndFile];
        _rEndByte = rTotalEndByte - preSize;
      }
    }
    // destroying rest (might be useful if the list is big)
    //std::cout<<this->par().rank()<<std::endl;
    //std::cout<<"begin at: "<<_rBeginFile<<std::endl;
    //std::cout<<"end at: "<<_rEndFile<<std::endl;
    _rEndFile -= _rBeginFile;
    for (int i = 0; i <= _rEndFile; i++) {
      _props.fnames[i] = _props.fnames[i + _rBeginFile];
    }
    _rBeginFile = 0;
    _props.fnames.resize(_rEndFile + 1);
  }

private:

  void _headerCols(std::vector<int>& cols, const std::vector<std::string>& headers) {
    std::string fname; 
    if (!_props.fnames.empty()) {
      fname = _props.fnames[0]; 
    } else {
      auto fnames = vglob(_props.fpat, 1);
      if (!fnames.empty()) fname = fnames[0];
    }
    if (!fname.empty()) {
      std::ifstream f(fname);
      if(f.is_open()) {
        std::string line;
        std::getline(f, line, _props.rDelim);
        std::vector<std::string> vstr;
        if (_props.cDelims != "none") {
          boost::split(vstr, line, boost::is_any_of(_props.cDelims),
                       boost::token_compress_on);
        } else {
          vstr.push_back(line);
        }
        for(const auto& head : headers) {
          auto it = std::find(std::begin(vstr), std::end(vstr), head);
          if (it == std::end(vstr)) break;
          else cols.push_back(it - std::begin(vstr) + 1);
        }
      }
    }
    if (cols.size() != headers.size()) {
      cols.clear();
      Karta::inst().log(
          "header list provided for load can not be read from file: " +
              fname,
          LogMode::warning);
    }
  }

  void _sanityCheck() {
    _idealSize = (_props.cols.empty()) ? osize : 0;
    // selection can be a boolean mask or index numbers
    // ideal size is used for filling null or strict schema
    // ideal size is the size of the row to be extracted before
    // applying slct
    _isMask = true;
    for (auto it : _props.cols) {
      if (it > _idealSize) _idealSize = it;
      if (it > 1) _isMask = false;
    }
    if (!_props.cols.empty() && _isMask) {
      _idealSize = _props.cols.size();
      int rowSize = 0;
      for (auto it : _props.cols) {
        if (it == 1) rowSize++;
      }
      assert(rowSize == osize && "number of columns in select list and row \
pes are different.");
    } else if (!_props.cols.empty()) {
      assert(_props.cols.size() == osize && "number of columns in select list and \
row types are different.");
      for (auto it : _props.cols) {
        assert(it > 0 && "Select list indexing starts from 1");
      }
      auto uniq = std::set<int>(_props.cols.begin(), _props.cols.end());
      assert(uniq.size() == _props.cols.size() && "Dublicate column in select list.");
    }
  }

  void _divideFiles (int pos, std::vector<int> procs) {
    auto share = int(_props.fnames.size() / procs.size());
    if (share == 0) share = 1;
    _rBeginFile = share * pos;
    _rEndFile = share * (pos + 1) - 1;
    if (_rEndFile > int(_props.fnames.size() - 1) || pos == int(procs.size()) - 1) {
      _rEndFile = _props.fnames.size() - 1;
    }
    // removing not required fnames from the list
    if (_rBeginFile > int(_props.fnames.size() - 1)) {
      _props.fnames.empty();
      _rBeginFile = -1;
    } else {
      _rEndFile -= _rBeginFile;
      for (auto i = 0; i <= _rEndFile; i++) {
        _props.fnames[i] = _props.fnames[i + _rBeginFile];
      }
      _rBeginFile = 0;
      _props.fnames.resize(_rEndFile + 1);
    }
  }
  bool _sizeCheck(std::vector<std::string> &vstr) {
    if ((_isMask && int(vstr.size()) != _idealSize) ||
        (!_isMask && int(vstr.size()) < _idealSize)) {
      if (!_props.strict) {
        vstr.resize(_idealSize);
      } else {
        return false;
      }
    }
    return true;
  }

  std::pair<bool, rs> _processLine(std::string line) {
    std::vector<std::string> vstr;
    if (!_props.cDelims.empty()) {
      boost::split(vstr, line, boost::is_any_of(_props.cDelims),
                   boost::token_compress_on);
    } else {
      vstr.push_back(line);
    }
    // delimiters may leave empty strings at the ends
    if (!vstr.empty() && vstr[vstr.size() - 1].empty()) vstr.pop_back();
    if (!vstr.empty() && vstr[0].empty()) vstr.erase(std::begin(vstr));
    if (_props.addFileName) {
      std::string name = std::string(_props.fnames[_cur]);
      vstr.push_back(std::move(name));
    }
    std::pair<bool, rs> st = std::make_pair(true, rs::br);
    // pre-check if given
    if (_props.check) {
      st = _props.check(vstr); // exception handling or not? check after profiling
      if (!st.first) { return st; }
    }
    if (!_props.drop.empty()) {
      std::sort(_props.drop.begin(), _props.drop.end(), std::greater<int>());
      for (auto it : _props.drop) {
        if (it <= int(vstr.size())) vstr.erase(std::begin(vstr)+it-1);
      } 
    }
    if (!_sizeCheck(vstr)) {
      st.first = false;
      return st;
    }
    if (!_props.cols.empty()) {
      std::vector<std::string> temp;
      temp.reserve(osize);
      if (_isMask) {
        detail::stripContainerBool(vstr, temp, _props.cols);
      } else {
        detail::stripContainer(vstr, temp, _props.cols);
      }
      vstr = std::move(temp);
    }
    try {
      detail::meta::lexCastTuple(vstr, _out, _props.strict);
    } catch (boost::bad_lexical_cast) {
      st.first = false;
      return st;
    } catch (...) {
      st.first = false;
      return st;
    }
    return st;
  }

  bool _nextFile() {
    if (_pos == -1 || _rBeginFile == -1) return false;
    _cur++;
    while (_cur < int(_props.fnames.size())) {
      if (_cur >= _rBeginFile && _cur <= _rEndFile) {
        if (!_fb) _fb = std::make_unique<std::filebuf>();
        if (_fb->is_open()) _fb->close();
        _fb->open(_props.fnames[_cur], std::fstream::in); // TODO log error @haptork
        if (!_fb->is_open()) {
          Karta::inst().log("can not open file: "+_props.fnames[_cur], LogMode::warning);
          _cur++;
          continue;
        }
        _is = nullptr;
        _is = std::make_unique<std::istream>(_fb.get());
        if (!_props.tilleof && _cur == _rBeginFile) {
          (*_is).seekg(_rBeginByte);
          // the seek can start from the middle of a row, hence that row is
          // read in the prior process and ignored in the start of reading.
          if (_pos != 0) {
            char c;
            while ((*_is).get(c)) {
              if (_props.rDelim == 's') {
                if (c == ' ' || c == '\n' || c == '\r' || c == '\t') { break; }
              } else {
                if (c == _props.rDelim) break;
              }
            }
          }
        }
        return true;
      }
      _cur++;
    }
    return false;
  }

  bool _nextLine(std::string &line) {
    if (_props.rDelim == 's') {
      (*_is) >> line; // TODO: check how to modify ret
    } else {
      std::getline(*_is, line, _props.rDelim);
    }
    return true;
  }

  inline auto _lineHai() {
    using std::make_pair;
    if (_nextLine(_line) && !(*_is).eof()) {
      auto status = _processLine(_line);
      if (ksize && status.first) {
        curKey = detail::meta::slctTuple(_out, Kslct{});
      }
      auto isOverFlow =
          (!_props.tilleof && _cur == _rEndFile && (*_is).tellg() > _rEndByte);
      if (isOverFlow && ((status.second == rs::prior && preBreak) ||
                         (status.second == rs::ignore) ||
                         (status.second == rs::br && ksize && in &&
                          prepreBreak && status.first && curKey != preKey))) {
        return make_pair(rs::eof, false);
      }
      prepreBreak = preBreak;
      preBreak =  isOverFlow;
      if (status.first) {
        if (ksize && !in && (first || curKey == preKey) && _pos &&
            _cur == _rBeginFile && _rBeginByte != 0) {
          if (first) first = false;
          if (curKey != preKey) preKey = std::move(curKey);
          return make_pair(rs::ignore, false);
        }
        if (_props.rowsMax) _rowsRead++;
        if (ksize) {
          if (first) first = false;
          if (!in) in = true;
          if (curKey != preKey) preKey = std::move(curKey);
        }
      }
      if (status.second == rs::eod || (_props.rowsMax && _rowsRead >= _props.rowsMax)) {
        _props.fnames.clear();
        return make_pair(rs::eof, status.first);
      }
      if ((isOverFlow && status.second == rs::br && !ksize) ||
          status.second == rs::eof) {
        return make_pair(rs::eof, status.first);
      }
      return make_pair(rs::ignore, status.first);
    }
    return make_pair(rs::eof, false);
  }

  bool loaded = false;
  bool accept = false;

  FromFileProps _props;

  std::string _line;
  bool in = false;
  bool first = true;
  bool preBreak = false;
  bool prepreBreak = false;
  ktype curKey, preKey;

  I _out;
  int _cur{-1};
  std::unique_ptr<std::filebuf> _fb{nullptr};
  std::unique_ptr<std::istream> _is{nullptr};
  bool _isMask;
  int _idealSize;
  int _rBeginFile;
  int _rEndFile;
  long long _rBeginByte{0};
  long long _rEndByte{0};
  int _rowsRead{0};
  int _pos {-1};
};

template <class... Is>
auto fromFile(std::string fpat = "") {
  return FromFile<std::tuple<Is...>, detail::meta::slct<>>{fpat};
}

template <class... Is>
auto fromFile(std::vector<std::string> flist) {
  return FromFile<std::tuple<Is...>, detail::meta::slct<>>{flist};
}

} // namespace ezl

#endif // !FROMFILE_EZL_H
