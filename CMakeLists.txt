cmake_minimum_required(VERSION 2.8)
project(easyLambda)

set (easyLambda_VERSION_MAJOR 0)
set (easyLambda_VERSION_MINOR 2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_MPI "Enable parallelism using MPI" ON)

if(CMAKE_COMPILER_IS_GNUCC)
  option(ENABLE_COVERAGE "Enable coverage reporting for gcc/clang" FALSE)
  if(ENABLE_COVERAGE)
    set(ENABLE_MPI OFF)  # coverage is done without parallelism
    add_compile_options(--coverage -O0)
  else()
    add_compile_options(-O3)
  endif()
else()
  add_compile_options(-O3)
endif()

if (MSVC)
  add_compile_options(/W3)
else()
  add_compile_options(-Wall)
endif()

if(ENABLE_MPI)
  find_package(MPI)
  if(MPI_FOUND)
    find_package(Boost 1.58 COMPONENTS mpi serialization)
    if(Boost_FOUND)
      include_directories(include ${Boost_INCLUDE_DIR})
      link_directories(${Boost_LIBRARY_DIR})
    else()
      message("Boost mpi and serialization are required for MPI parallelism but not found. Continuing build without parallelism.")
      find_package(Boost 1.54 REQUIRED)
      include_directories(include ${Boost_INCLUDE_DIR})
      add_compile_options(-DNOMPI)
    endif()
  else()
    message("MPI not found. Continuing build without parallelism.")
    find_package(Boost 1.54 REQUIRED)
    include_directories(include ${Boost_INCLUDE_DIR})
    add_compile_options(-DNOMPI)
  endif() 
else()
  find_package(Boost 1.54 REQUIRED)
  include_directories(include ${Boost_INCLUDE_DIR})
  add_compile_options(-DNOMPI)
endif()

# set(EXAMPLES )
file(GLOB TESTS test/*.cpp)

add_executable(${PROJECT_NAME}_test ${TESTS})
target_link_libraries(${PROJECT_NAME}_test ${Boost_LIBRARIES} ${MPI_CXX_LIBRARIES})
target_include_directories(${PROJECT_NAME}_test PUBLIC test)

include(CTest)

add_test(NAME ${PROJECT_NAME}_test 
          COMMAND "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_test"
          WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
add_custom_target(check_test COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS ${PROJECT_NAME}_test)

file(GLOB EXAMPLES examples/*.cpp)
foreach(x ${EXAMPLES})
  get_filename_component(FNAME ${x} NAME_WE)
  add_executable(${FNAME} ${x})
  target_link_libraries(${FNAME} ${Boost_LIBRARIES} ${MPI_CXX_LIBRARIES})
  add_test(NAME ${FNAME}
            COMMAND "${CMAKE_BINARY_DIR}/${FNAME}"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
  add_custom_target(check_${FNAME} COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS ${FNAME})
endforeach()

