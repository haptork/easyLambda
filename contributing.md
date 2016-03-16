Contribution and feedback is highly appreciated.

There are a lot of areas that need work in this nascent project: compile-time &
run-time optimization, improving parallel performance, simplifying interface,
adding more algorithms for common data-analysis tasks. Adding examples and tests.

## Exciting additions and improvements:
 - Addition of UDFs that plot and animate rows as they stream possibly using Cinder.
 - Improvement on data-flow abstraction, by giving a cycle<N>() expression to create
   a link to the unit that is N above the current one. This will also result in
   better implementation of current flow expressions.
 - Using type lists or tuples in builder expressions to make the code cleaner and
   possibly bring down the compile time.
 - Improve parallelization for tuples with plain-old-datatypes (PODs) by detecting
   POD using type-traits and then sending a fixed size.
 - Join units to join streams with different data-types.
 - Logistic regression and other ML algorithms as built-in algorithms.

## Basic Design:
The project files are arranged in directories pipeline, mapreduce, builder,
helper and algorithms. The first three can be considered as three hierarchical
layers. The pipeline classes define the behaviour for pipeline architecture
with sources, destinations, links, root etc. The mapreduce classes add map,
reduce, filter, load, dump functionality by inheriting pipeline classes. This
layer uses helper meta functions for row and column manipulation, UDF calls
etc. Finally, the builder classes provide a builder expression abstraction for
building objects of mapreduce types and connect them in a pipeline. They govern
how the user uses the library. Algorithms directory has generic function
objects for use with rise, map, filter, reduce or reduceAll.

## Coding Style:
- Includes are grouped into standard library then other library includes
  and then ezl includes. Each group is alphabetically ordered.
- Naming convention is camelCase, capitalizing first letter for classes.
  However, it is nothing hard and fast. Private members are prefixed with underscore.
- Indentation is 2 spaces. No indentation is used for namespaces.
- Commenting and documenting is minimal with brief descriptions for every file
  and for most of the classes. Documentation can be generated with doxygen.

The coding style is not hard and fast for contributions. Code the way you like, 
consistent and clean.
