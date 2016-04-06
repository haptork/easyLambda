#include <ctorTeller.hpp>
#include <tuple>

#include <boost/mpi.hpp>


#include <unitTest/slctTest.cpp>
#include <unitTest/slctTupleTest.cpp>
#include <unitTest/funcInvokeTest.cpp>
#include <unitTest/MapTest.cpp>
#include <unitTest/FilterTest.cpp>
// ReduceTest build only in Apple clang due to some different implementatin of
// tie, nothing related to library. TODO: for gcc as well
//#include <unitTest/ReduceTest.cpp>
#include <unitTest/ReduceAllTest.cpp>
#include <unitTest/fromFileTest.cpp>
#include <unitTest/MPIBridgeTest.cpp>
#include <unitTest/LoadTest.cpp>


void slctTest();
void slctTupleTest();
void funcInvokeTest();

void MapTest(int, char*[]);
void ReduceTest(int, char*[]);
void ReduceAllTest(int, char*[]);
void FilterTest(int, char*[]);
void fromFileTest(int, char*[]);
void MPIBridgeTest(int, char*[]);
void LoadTest(int, char*[]);

int ctorTeller::_ctor = 0;
int ctorTeller::_copyCtor = 0;
int ctorTeller::_moveCtor = 0;
int ctorTeller::_copyAssign = 0;
int ctorTeller::_moveAssign = 0;

int main(int argc, char* argv[]) {
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator comm;

  slctTest();
  slctTupleTest();
  funcInvokeTest();
  MapTest(argc, argv);
  //ReduceTest(argc, argv);
  ReduceAllTest(argc, argv);
  FilterTest(argc, argv);
  fromFileTest(argc, argv);
  MPIBridgeTest(argc, argv);
  LoadTest(argc, argv);

  std::cout<<"All tests passed successfully"<<std::endl;
  return 0;
}
