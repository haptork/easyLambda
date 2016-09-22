---
title: "Getting Started"
permalink: /docs/quick-start-guide/
excerpt: "Installation and basic usage."
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

Easylambda is a header only libray. The library can be downloaded
from the repository ([download
link](https://github.com/haptork/easyLambda/archive/master.zip)). Follow the
instructions given below for quick installation and usage.

## Requirements
 - C++14 compliant compiler. 
   - Tested with gcc-5.3, gcc-6.0, Apple LLVM version 7.0.0 (clang-700.0.72).
 - C++ Boost library 1.58 or later with boost-mpi.
 - MPI wrapper compiler (mpic++/mpicxx) and mpirun.

## Installing
Extract the downloaded zip and place the contents of the include directory in
your compiler include path so that it is available to the compiler. If you do
not add the directory in the include path then you can give the path of the
include directory with compiler flag -I.

## Compiling
There are no linking requirements of ezl library but it uses boost::serialization
and boost::mpi that need to be linked.
Here is how you can compile a program in general:
`mpic++ file.cpp -Wall -std=c++14 -O3 -I path_to_ezl_include -lboost_mpi -lboost_serialization`

Replace path_to_ezl_include with the actual path of ezl include directory in your
system. If you have added the contents of include directory to your source tree
or global include path then you don't need to pass -I path_to_ezl_include flag.

You can compile unit-tests with `make unittest` and run with `./bin/unittest`.

You can compile an example using make with `make example fname=name`, replace
name with the name of the example file for e.g. `make example fname=wordcount`
to compile wordcount example.

## Running

After compiling, the executable can be run with mpirun 
`mpirun -n 4 path_to_exe args…` or simply as `path_to_exe args…`.

## Using ezl

For all of the core functionality only `ezl.hpp` is needed to be included in
the program. To use generic function objects like `ezl::count` etc header
files from `ezl/algorithms/` directory need to be included. The function
objects are grouped according to their use case. The algorithms for use with
reduce are in reduces.hpp, with filter are in filters.hpp and so on. 

There are
many examples and demonstrations given in the examples directory, pick an
example of your interest to begin with. You can check reference for information
on a specific easyLambda call. Learn by example gives a humble introduction to
ezl with examples showing many essential features.
