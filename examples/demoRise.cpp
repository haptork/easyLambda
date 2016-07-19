/*!
 * @file
 * demo for reduceAll.
 *
 * For library functions provided for rise see `demoFromFile` and `demoIO`.
 *
 * For demonstration the pipelines are not built or run.
 * Add .run() at the end of a flow and add .dump() in any unit to check results.
 * */
#include <array>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <vector>

#include <boost/mpi.hpp>

#include <ezl.hpp>

class Op {
public:
  auto operator () () {
    isMore = !isMore;
    return std::tie(pos, isMore);
  }

  // The pair is passed at the beginning of data flow.
  // The pair contains pos, ranks of processes the unit is running in pos is
  // from (0, procInfo.second.size() - 1) which is different for every proc.
  // The rank of current process is at index pos in the ranks vector.
  void operator () (std::pair<int, std::vector<int>> procInfo) {
    pos = procInfo.second[procInfo.first];
  }
private:
  int pos;
  bool isMore {true};
};

void demoRise() {
  // returning a vector to return multiple rows.
  // An empty vector marks the end of data.
  std::array<int, 3> ar{{1,2,3}};
  ezl::rise([&ar]() {
    return std::move(ar);
  });

  // returning a row at a time in a pair/tuple(with a tie).
  // The get<0>/first is row and get<1>/second is bool.
  // If bool=false marks end of data, row that is returned with
  // false is also ignored.
  int cur = 0;
  constexpr auto max = 100;
  ezl::rise([&cur, &max]() {
    cur++;
    return std::make_pair(cur, cur<=max);
  }).prll(1)
  .filter([](int) { return true; }).prll(1., ezl::llmode::task);

  // A function object that uses () (pair<int, vector<int>>) for process-info
  ezl::rise(Op());


  // shows how process-info can be used with lambda or c-style functions
  std::pair<int, std::vector<int>> procInfo;
  auto& pos = procInfo.first;
  bool isMore = false;
  ezl::rise([&pos, &isMore]() {
    isMore = !isMore;
    return std::tie(pos, isMore);
  }).procDump(procInfo);
  // The pair is filled with process information at the beginning of run().
  // The pair contains pos, ranks of processes the unit is running in pos is
  // from (0, procInfo.second.size() - 1) which is different for every proc.
  // The rank of current process is at index pos in the ranks vector.
}

int main(int argc, char *argv[]) {
  boost::mpi::environment env(argc, argv, false);
  try {
    demoRise();
  } catch (const std::exception& ex) {
    std::cerr<<"error: "<<ex.what()<<'\n';
    env.abort(1);
  } catch (...) {
    std::cerr<<"unknown exception\n";
    env.abort(2);
  }
  return 0;
}
