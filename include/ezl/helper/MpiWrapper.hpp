#ifndef MPI_WRAPPER_EZL_H
#define MPI_WRAPPER_EZL_H

#ifndef NOMPI
#include <boost/mpi.hpp>
namespace ezl {
namespace detail {
class CommWrapper {
public:
  auto rank() { return _comm.rank(); }
  auto size() { return _comm.size(); }
  const auto &internal() const { return _comm; }
private:
  boost::mpi::communicator _comm;
};
}
class Env {
public:
  Env(int argc, char* argv[], bool exceptHandle) : _env{argc, argv, exceptHandle} {}
  Env() : _env{} {}
  auto abort(int signal) {
    return _env.abort(signal);
  }
  const auto &env() {
    return _env;
  }
private:
  boost::mpi::environment _env;
};
}
#else
namespace ezl {
namespace detail {
class CommWrapper {
public:
  auto rank() { return 0; }
  auto size() { return 1; }
  const auto &internal() const { return *this; }
};
}
class Env {
public:
  Env(int argc, char* argv[], bool exceptHandle) {}
  Env() {}
  auto abort(int signal) {
    return signal;
  }
  const auto &env() { return *this; }
};
}
#endif
#endif