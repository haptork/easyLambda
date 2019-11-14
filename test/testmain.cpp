#include <ezl.hpp>
#include <ctorTeller.hpp>

namespace ezl {
namespace test {
void slctTest();
void slctTupleTest();
void funcInvokeTest();

void MapTest(int, char*[]);
void ReduceTest(int, char*[]);
void ReduceAllTest(int, char*[]);
void FilterTest(int, char*[]);
void fromFileTest(int, char*[]);
void MPIBridgeTest(int, char*[]);
void RiseTest(int, char*[]);

int ctorTeller::_ctor = 0;
int ctorTeller::_copyCtor = 0;
int ctorTeller::_moveCtor = 0;
int ctorTeller::_copyAssign = 0;
int ctorTeller::_moveAssign = 0;

int unittests(int argc, char* argv[]) {
  slctTest();
  slctTupleTest();
  funcInvokeTest();
  MapTest(argc, argv);
  ReduceTest(argc, argv);
  ReduceAllTest(argc, argv);
  FilterTest(argc, argv);
  fromFileTest(argc, argv);
  RiseTest(argc, argv);
#ifndef NOMPI
  MPIBridgeTest(argc, argv);
#endif
  std::cout<<"All tests passed successfully\n";
  return 0;
}
}
}

int main(int argc, char* argv[]) {
  ezl::Env env{argc, argv, false};
  ezl::test::unittests(argc, argv);
  return 0;
}