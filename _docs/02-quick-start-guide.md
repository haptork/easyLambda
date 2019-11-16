---
title: "Getting Started"
permalink: /docs/quick-start-guide/
excerpt: "Installation and basic usage."
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

Easylambda is a header only libray. The library can be downloaded
from the repository ([download
link](https://github.com/haptork/easyLambda/archive/master.zip)). The
application that includes the library can be built with parallelism features
or without. In non-parallel mode the compilation does not require MPI and boost
libraries, making it easier to try and test the library. Follow the
instructions given below for quick installation and usage.

## Requirements
 - C++14 compliant compiler. 
   - Tested with gcc-6.0, Apple LLVM version 7.0.0 (clang-700.0.72).
 - For parallel build: C++ Boost library 1.58 or later with boost-mpi else boost 1.55 or later header only is sufficient.
 - For parallel build: MPI wrapper compiler (mpic++/mpicxx) and mpirun.

## Getting on AWS Elastic
 - Use ami `ami-1bad0978` publicly available on Asia-Pacific(Singapore). The ami can be
   launched for a single node or in a cluster of nodes with [starcluster](http://star.mit.edu/cluster/docs/latest/quickstart.html).
 - SSH with username `ubuntu`.
 - Clone the easyLambda github repository in the home directory and you are
   ready to compile and run the examples or your applications.

## Installing
Extract the downloaded zip and place the contents of the include directory in
the compiler include path so that it is available to the compiler. If the directory
is not added to the include path then the path of the include directory with
compiler flag -I can be given while compiling.

* To install newest gcc on Ubuntu or related distros, check [link](http://askubuntu.com/questions/428198/getting-installing-gcc-g-4-9-on-ubuntu).
* For parallel builds: To install boost with mpi follow the [link](http://www.boost.org/doc/libs/1_61_0/doc/html/mpi/getting_started.html).

## Compiling
There are no linking requirements of ezl library but in parallel build it uses boost::serialization
and boost::mpi that need to be linked.
Here is how to compile a program in general:

- For non parallel compilation: `g++ file.cpp -Wall -std=c++14 -DNOMPI -O3 -I path_to_ezl_include`
- For parallel compilation: `mpic++ file.cpp -Wall -std=c++14 -O3 -I path_to_ezl_include -lboost_mpi -lboost_serialization`

Replace path_to_ezl_include with the actual path of ezl include directory in your
system. If you have added the contents of include directory to your source tree
or global include path then you don't need to pass -I path_to_ezl_include flag.

For non parallel build `-DNOMPI` flag is added to the compilation and linking to boost
libraries is not required.

To build tests and examples any of the two CMake or Make can be used.

For using CMake follow the following steps:

- Make a new directory `_build` or any other name inside the easyLambda main project directory. Go to this directory and run `cmake ..` for build with parallelism or `cmake -DENABLE_MPI=NO ..` for non parallel build.
- Run `cmake --build .`. Now you can run any test or example that are built. You can use `mpirun` to run examples
if built with parallelism.

For compiling with Make on unix based systems:

- Unit tests can be compiled with `make bin/test` and run with `./bin/test`.

- You can compile an example using make with `make example fname=name`, replace
name with the name of the example file for e.g. `make example fname=wordcount`
to compile wordcount example. You can compile all the examples with `make examples`.

## Running

After compiling, the executable can be run with mpirun 
`mpirun -n 4 path_to_exe args…` or simply as `path_to_exe args…`.

## Using ezl

For all of the core functionality only `ezl.hpp` is needed to be included in
the program. To use generic function objects like `ezl::count` etc header
files from `ezl/algorithms/` directory need to be included. The function
objects are grouped according to their use case. The algorithms for use with
reduce are in reduces.hpp, with filter are in filters.hpp and so on. 

There are many examples and demonstrations given in the [examples directory](https://github.com/haptork/easyLambda/tree/master/examples/),
pick an example of your interest to begin with. You can check [reference]({{ site.url }}{{ site.baseurl }}/docs/starting-dataflow/) for
information on a specific easyLambda call. [Learn by example]({{ site.url }}{{ site.baseurl }}/docs/hello-world/) gives a humble
introduction to ezl with examples showing many essential features.

For any help please let us know via mail or github issues.
