/*!
 * @file
 * class MPIBRIDGE, unit for MPI parallelism
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef __MPIBRIDGE_SIMR__
#define __MPIBRIDGE_SIMR__

#include <iostream>
#include <map>
#include <tuple>
#include <vector>

#include <boost/mpi.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>

#include <ezl/pipeline/Bridge.hpp>
#include <ezl/helper/Karta.hpp>
#include <ezl/helper/meta/slctTuple.hpp>
#include <ezl/helper/meta/serializeTuple.hpp>
#include <ezl/helper/Par.hpp>

namespace ezl {
namespace detail {

/*!
 * @ingroup mapreduce
 * A pipeline link for task and data parallelism using MPI.
 * Dest receives the data that is then sent to diffrent processes using parallel
 * information `Par` of the task.
 *
 * As the data is received it is sent async if the prior send request for that
 * process has been served, at the end receive requests are also checked for
 * new data from all the sending processes if the task is in receiving process.
 *
 * The implementation tries to optimize the communication using different
 * procedures. The data is buffered to some extent before sending to avoid
 * eager messages which may result in overflowing of MPIBuffer and an error, 
 * however a small number of eager messages are sent without buffering.
 *
 * To a buffer size limit, if the async send is not finished, the process
 * continues receiving and buffering the data. When maximum buffer size
 * is reached the process waits for the data to be received. This may result
 * in a deadlock in specific pipelines.
 *
 * send buffers and receive buffers are allocated per sender/receiver.
 *
 * All the requests have a counter that increases or decreases by a factor
 * if prior request is stuck or cleared respectively. It is to ensure that
 * test is called less frequently and a partial sync is there in between the
 * communicating processes. There can be better ways to achieve this, however.
 *
 * This is added to the pipeline when a unit specifies `prll` property.
 * `ParExpr` has the builder expression for adding it before the current unit
 * in the pipeline. For direct use see unittests.
 *
 * */
template <class IO, class Kslct, class HashScheme>
struct MPIBridge : public Bridge<IO> {
public:
  using itype = IO;
  using otype = IO;
  using buftype = typename meta::SlctTupleType<IO>::type;
  static constexpr int osize = std::tuple_size<otype>::value;
  using ktype = typename meta::SlctTupleType<buftype, Kslct>::type;

private:
  // struct for keeping record of each sender
  struct Sender {
    std::vector<boost::mpi::request> reqs; // multiple reqs for different tags
    buftype buf;
    std::vector<buftype> vBuf;
    int sig{0};
    size_t counter{1};
    size_t tick{0};
  };

  // struct for keeping record of each receiver
  struct Receiver {
    std::vector<buftype> sentBuf;
    std::vector<buftype> buffer;
    boost::mpi::request req;
    bool sigged{false};
    size_t counter{1};
    size_t tick{0};
    size_t nEagerMsg{0};
    bool isFirst{true};
    ktype preKey;
    ktype curKey;
    //typename meta::SlctTupleRefType<buftype, Kslct>::type curKey;
  };

public:
  MPIBridge(ProcReq r, bool toAll, bool ordered)
      : Bridge<IO>{r}, _toAll{toAll}, _ordered{ordered} {}

  // expects  this->parHandle() != nullptr &&
  //          this->parHandle()->inRange() && this->par().nProc()>0
  virtual void dataEvent(const itype &data) final override {
    //buftype a = std::make_tuple("hi", 4);
    //IO a = 4;
    const auto &par = this->par();
    if (!this->parHandle() || !this->parHandle()->inRange()) return;
    if (_toAll) {
      for(const auto& it : par) {
        _recvrs[it].buffer.push_back(data);
        _sendSafe(it);
      }
      _recvAll();
      return;
    }
    auto target = par.rank();
    if (par.nProc() == 1) {
      target = par[0];
    } else if (std::tuple_size<ktype>::value == 0) {
      target = par[_curRoll];
      _curRoll = (_curRoll + 1) % par.nProc();
    } else {
      auto key = meta::slctTupleRef(data, Kslct{});
      target = par[(_hash(key)) % par.nProc()];
      if (_ordered) _recvrs[target].curKey = std::move(key);
    }
    _recvrs[target].buffer.push_back(std::move(data));
    if (_ordered && !_orderedPass(target)) return;
    _sendSafe(target); 
  }

  // expects  this->parHandle() != nullptr &&
  //          this->parHandle()->inRange() && this->par().nProc()>0
  virtual void dataEvent(const std::vector<itype> &vdata) final override {
    if (vdata.empty()) return;
    auto &par = this->par();
    if (_toAll) {
      for(const auto& it : par) {
        _recvrs[it].buffer.insert(std::end(_recvrs[it].buffer), std::begin(vdata), std::end(vdata));
        _sendSafe(it);
      }
      _recvAll();
      return;
    }
    if (par.nProc() == 1) {
      std::move(std::begin(vdata), std::end(vdata),
                std::back_inserter(_recvrs[par[0]].buffer));
      if (_ordered && !_orderedPass(par[0])) return;
      _sendSafe(par[0]);
    } else {
      std::set<int> dirty;
      for (const auto& it : vdata) {
        auto target = par.rank();
        if (std::tuple_size<ktype>::value == 0) {
          target = par[_curRoll];
          _curRoll = (_curRoll + 1) % par.nProc();
        } else {
          auto key = meta::slctTupleRef(it, Kslct{});
          target = par[(_hash(key)) % par.nProc()];
        }
        _recvrs[target].buffer.push_back(std::move(it));
        if (_ordered) {
          if (_orderedPass(target)) _sendSafe(target);
        } else {
          dirty.insert(target);
        }
      }
      if (_ordered) return;
      for (auto &it : dirty) _sendSafe(it);
    }
  }

private:
  virtual void _dataBegin() override final {
    _recvrsInit();
    _sendrsInit();
    std::vector<buftype> temp;
    _maxSendbuf = temp.max_size() / _minSendBufLimit; // max is divided by min
    _minSendbuf = temp.max_size() / _maxSendBufLimit; // min is divided by max
    if(_maxSendbuf < _sendBufLower) _maxSendbuf = _sendBufLower;
    else if (_maxSendbuf > _maxSendBufLimit) _maxSendbuf = _maxSendBufLimit;
    if(_minSendbuf < _sendBufLower) _minSendbuf = _sendBufLower;
    else if (_minSendbuf > _sendBufLower) _minSendbuf = _sendBufLower;
  }

  virtual void _dataEnd(int j) override final {
    _sig = j;
    if (!this->par().inRange() && !this->parHandle()->inRange()) {
      return;
    }
    bool toSend = true;
    while (toSend || !_sendrs.empty()) { // no busy wait
      if (toSend) toSend = _sendAll(); // sends signals as well
      if (!_sendrs.empty()) _recvAll();
    }

    _curRoll = 0;
    _recvrs.clear(); // on right, to which data is to be sent
    _sendrs.clear();   // on left, from which data is received
    _minRecvCounter = 1;
    _minRecvIndex = 0;
  }

  void _recvrsInit() {
    if (this->parHandle()->inRange()) { // there are links on left
      for (const auto& it : this->par()) {
        if (_recvrs.find(it) == std::end(_recvrs)) _recvrs[it] = Receiver(); 
      }
    }
  }

  void _sendrsInit() {
    if (!this->par().inRange()) return;
    auto comm = Karta::inst().comm();
    for (auto it : *(this->parHandle())) {
      if (it == this->parHandle()->rank()) continue;
      if (_sendrs.find(it) == std::end(_sendrs)) {
        _sendrs[it] = Sender();
        _sendrs[it].reqs = std::vector<boost::mpi::request>{
            comm.irecv(it, this->par().tag(1), _sendrs[it].buf),
            comm.irecv(it, this->par().tag(2), _sendrs[it].vBuf),
            comm.irecv(it, this->par().tag(0), _sendrs[it].sig)};
      }
    }
  }

  
  bool _orderedPass(int target) {
    if (this->parHandle()->nProc() == 1) return true;
    if (_recvrs[target].isFirst) {
      _recvrs[target].isFirst = false;
      _recvrs[target].preKey = std::move(_recvrs[target].curKey);
      return false;
    } else if (_recvrs[target].preKey == _recvrs[target].curKey) {
      return false; 
    } else {
      _recvrs[target].preKey = std::move(_recvrs[target].curKey);
    }
    return true;
  }

  bool _recv(const typename std::map<int, Sender>::iterator &it, int maxIters) {
    //auto req = boost::mpi::test_any(std::begin(it->second.reqs),
    //                                std::end(it->second.reqs));
    auto req = std::begin(it->second.reqs);
    auto reqIndex =  0;
    while (req != std::end(it->second.reqs)) {
      if (req->test()) {
        break;
      }
      req++;
      reqIndex++;
    }
    if (req == std::end(it->second.reqs)) return false;
    auto iters = 0;
    switch (reqIndex) {
    case 0:
      do {
        auto hold = std::move(it->second.buf);
        *(req) = Karta::inst().comm().irecv(
            it->first, this->par().tag(1), it->second.buf);
        for (auto &it : this->next()) {
          it.second->dataEvent(hold);
        }
      } while ((!maxIters || iters++ < maxIters) && req->test());
      break;
    case 1:
      do {
        auto vhold = std::move(it->second.vBuf);
        *(req) = Karta::inst().comm().irecv(
            it->first, this->par().tag(2), it->second.vBuf);
        for (auto &it : this->next()) {
          for (auto &hold : vhold) it.second->dataEvent(hold);
        }
        if (_debug && vhold.size() >= _every) {
          std::cout << this->par().rank() << ", " << it->first << "- Vrecvd "
                    << vhold.size() << std::endl;
        }
      } while ((!maxIters || iters++ < maxIters) && req->test());
      break;
    default:
      it->second.reqs.pop_back();
      _recv(it, 0); // cross-check for eager messages
      it->second.reqs[0].cancel();
      it->second.reqs[1].cancel();
    }
    return true;
  }

  bool _recvAll(bool counterCheck = true) {
    if (_sendrs.empty()) return true; // implies !this->_par.inRange()
    auto it = std::begin(_sendrs);
    bool res = false;
    while(it != std::end(_sendrs)) {
      if (counterCheck) {
        it->second.tick++;
        // normalized check
        if (it->second.tick < it->second.counter / _minRecvCounter) {
          it++;
          continue;
        }
        if(_debug && it->second.tick>_every) {
          std::cout<<this->par().rank()<<"RecievCount: "<<it->first<<", "
            <<it->second.tick<<", "<<std::endl;
        }
        it->second.tick = 0;
      }
      auto ret = _recv(it, _maxItersRecv);
      if (!ret && counterCheck) {
        it->second.counter *= _incRecvCounter;
        if (it->second.counter > _maxCounter) {
          it->second.counter = _maxCounter;
        }
        if (_minRecvIndex == it->first) {
          for(const auto& jt : _sendrs) {
            if (jt.second.counter < _minRecvCounter) {
              _minRecvCounter = jt.second.counter;
              _minRecvIndex = jt.first;
            }
          }
        }
      } else if(counterCheck) {
          it->second.counter /= _decRecvCounter;
          if (it->second.counter == 0) it->second.counter = 1;
          if (it->second.counter < _minRecvCounter) {
            _minRecvCounter = it->second.counter;
            _minRecvIndex = it->first;
          }
      }
      if (ret && it->second.reqs.size()<=2) { // sigrcvd and no eager msg
        it = _sendrs.erase(it);
      } else {
        it++;
      }
      if (ret) res = ret;
    }
    return res;
  }

  void _sendSafe(int target) {
    auto len = _recvrs[target].buffer.size();
    if (!_send(target, !(_ordered && len>_minSendbuf))) {  // success
      _recvAll();
    } else {
      if (len < _maxSendbuf) {
        _recvAll();
      } else {
        Karta::inst().log("Receive process(es) are overflowing with data. For\
 better performance allocate more processes for receiving end compared to\
 sending end. Please note reduce operations receive data by default.");
        if (_sendrs.empty()) {  // nothing to receive 
          _recvrs[target].req.wait(); 
          //std::cout<<"wait over, sending: "<<_recvrs[target].buffer.size()<<std::endl;
          _recvrs[target].sentBuf = std::move(_recvrs[target].buffer);
          _recvrs[target].buffer.clear();
          _recvrs[target].req = Karta::inst().comm().isend(
            target, this->par().tag(2), _recvrs[target].sentBuf);
        } else {
          auto checkCount = true;
          while(_send(target, checkCount)) checkCount = _recvAll();
        }
      }
    }
  }

  /*!
   * Send buffer data to target process rank. Also, helps in checking
   * if the prior request have been served.
   *
   * @param counterCheck whether to check delayed based on counter.
   *
   * @return If false then nothing to be done with send unless new buffer
   *         value gets in.
   *
   * */
  bool _send(int target, bool counterCheck = true) {
    auto len = _recvrs[target].buffer.size();
    if (len == 0 && _recvrs[target].sentBuf.size() == 0) return false;
    if (target == this->parHandle()->rank()) {
      if (len == 1) {
        for (auto &it : this->next()) {
          it.second->dataEvent(_recvrs[target].buffer[0]);
        }
      } else {
        for (auto &it : this->next()) {
          for (auto &val : _recvrs[target].buffer) it.second->dataEvent(val);
        }
      }
      _recvrs[target].buffer.clear(); // TODO: needed?
      return false;
    }
    if (counterCheck && _recvrs[target].nEagerMsg == _maxEagerMsg &&
        len < _minSendbuf) {
      return false;
    }
    if (counterCheck) {
      _recvrs[target].tick++;
      if(_recvrs[target].tick < _recvrs[target].counter) return true;
      if(_debug && _recvrs[target].tick > _every) {
        std::cout<<this->par().rank()<<", "<<target<<" SendTick: "
          <<_recvrs[target].tick<<", "<<_recvrs[target].buffer.size()<<std::endl;
      }
      _recvrs[target].tick = 0;
    }
    if (_recvrs[target].req.test()) {
      if(len == 0) {
        _recvrs[target].sentBuf.clear();
        _recvrs[target].counter = 1;
        return false;
      }
      _recvrs[target].sentBuf = std::move(_recvrs[target].buffer);
      _recvrs[target].buffer.clear();
      if (len == 1) {
        _recvrs[target].req = Karta::inst().comm().isend(
            target, this->par().tag(1), _recvrs[target].sentBuf[0]);
        //_recvrs[target].dataSent++;
      } else {
        _recvrs[target].req = Karta::inst().comm().isend(
            target, this->par().tag(2), _recvrs[target].sentBuf);
        //_recvrs[target].dataSent+=_recvrs[target].sentBuf.size();
        if(_debug && _recvrs[target].sentBuf.size()>_every) {
          std::cout<<this->par().rank()<<", "<<target
          <<"- Vsent "<<_recvrs[target].sentBuf.size()<<std::endl;
        }
      }
      if (counterCheck) {
        _recvrs[target].counter /= _decSendCounter;
        if (_recvrs[target].counter == 0) _recvrs[target].counter = 1;
      }
      if (_recvrs[target].nEagerMsg < _maxEagerMsg) {
        _recvrs[target].nEagerMsg++;
      } else {
        _recvrs[target].nEagerMsg = 0;
      }
      return true;
    } else {
      if (counterCheck) {
      _recvrs[target].counter *= _incSendCounter;
      if (_recvrs[target].counter > _maxCounter) {
        _recvrs[target].counter = _maxCounter;
      }
      }
    }
    return true;
  }

  // if all the requests are clear and buffers empty then returns true
  bool _sendAll() {
    if (!this->parHandle()->inRange()) { // no recvrs 
      return false;
    }
    bool res = false;
    for (auto &it : _recvrs) {
      if (it.first == this->par().rank()) continue;
      if (_send(it.first, false)) {
        res = true;
      } else if(!it.second.sigged) {
        it.second.req = Karta::inst().comm().isend(
            it.first, this->par().tag(0), _sig);
        it.second.sigged = true;
        res = true;
      }
    }
    return res;
  }

private:
  const HashScheme _hash{};
  const bool _toAll;
  const bool _ordered;

  static constexpr auto _maxCounter = 1<<16;
  static constexpr auto _decSendCounter = 4; // divide sendCounter by on success
  static constexpr auto _incSendCounter = 2; // multiply sendCounter by on failure 
  static constexpr auto _decRecvCounter = 4; // divide recvCounter by on success
  static constexpr auto _incRecvCounter = 1.5; // multiply recvCounter by on failure 
  static constexpr auto _maxItersRecv = 1<<10; // maximum iterations if a process keeps receiving
  static constexpr bool _debug = false;  // dump extra info
  static constexpr size_t _every = 1<<21; // for dumping info in debug
  static constexpr auto _maxEagerMsg = 1<<8;  // eager msgs sent initially
  static constexpr size_t _maxSendBufLimit = 1<<30;
  static constexpr size_t _minSendBufLimit = 1<<20;
  static constexpr size_t _sendBufLower = 1<<10;

  size_t _maxSendbuf;
  size_t _minSendbuf;

  int _sig;
  int _curRoll{0};
  std::map<int, Receiver> _recvrs; // on right, to which data is to be sent
  std::map<int, Sender> _sendrs;   // on left, from which data is received
  size_t _minRecvCounter{1};
  int _minRecvIndex{0};
};
}
} // namespace ezl ezl::detail

#endif //!__MPIBRIDGE_SIMR__
