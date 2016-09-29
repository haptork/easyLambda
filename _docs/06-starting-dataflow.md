---
title: "Starting A Data Flow"
permalink: /docs/starting-dataflow/
excerpt: "Refernce on ways to start a dataflow"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}


A dataflow can either begin with a rise unit, a flow expression with input
stream columns as template arguments or a flow expression with a prior dataflow
as a parameter to continue building from it. To it new units can be added or
current unit can be modified with the properties in any order or modify the
output streaming routes using dataflow properties as shown in next section.

## ezl::rise

  ezl::rise is the original data source in a dataflow. It is a higher order
  function i.e. it takes another function as input that is called till it
  signals EndOfData. The function returns {row, isEndOfData} or a vector of
  rows in which case EndOfData is implied by returning an empty vector. Check
  the [code
  example](https://github.com/haptork/easyLambda/blob/e496a3e3070b806e8c48124d3454543c4cebc9b7/examples/demoRise.cpp)
  for more. The rise function streams the rows across all the branches of the
  dataflow when the dataflow runs. It doesn't buffer the rows.

  - Properties:
    1. [prll]({{ base_path }}/docs/prll-expr/)
    2. [dump]({{ base_path }}/docs/dump-expr/)

  Easylambda has function objects for rise given in ezl/algorithms/io.hpp such as
  for loading tabulated data from files, for loading file names from glob
  pattern, for loading list of data from a container (vector, array etc.), for
  calling the next unit n number of times without any parameter. All of these
  function can distribute the input rows among the processes when run in
  parallel. More on these function objects can be found under algorithms for
  rise section [here]({{ base_path }}/docs/rise-algo/).

## ezl::flow

  If a dataflow is not being started with a rise then it can either be started
  as continuation of another dataflow or it can be started by declaring the
  data-types of its input parameters without any source. A dataflow that has no
  source does not do anything when run. 

  - Properties (only if continuing from prior flow): no properties.

  {% highlight cpp %}
  auto fl = ezl::flow<char, int>()
              .map<2>([](int i) { return i * i; })
              .build();
  // fl is shared_ptr of type:
  // ezl::Flow<tuple<char, int>, tuple<char, int, int>>
  // The first tuple has cols of input stream and second has columns of output
  // A flow can be built and returned from a function.

  auto fl2 = ezl::flow(fl)
               .filter([](char, int, int) { 
                 return true; 
               })
              .run(); // doesn't do anything as there isn't a rise yet
  // fl2 is shared_ptr of type:
  // ezl::Flow<tuple<char, int, int>, tuple<char, int, int>>
  {% endhighlight %}
  
  An `ezl::Flow` object can be added to a flow with `addFlow` or `branchFlow`
  given that the output columns match the input columns of the Flow object or
  some other object can be added to recieve the output stream of the Flow object
  with `ezl::flow(...)` as done in above example. Check the next section for
  more on these.
  
  Check the [code
  example](https://github.com/haptork/easyLambda/blob/e496a3e3070b806e8c48124d3454543c4cebc9b7/examples/demoFlow.cpp)
  for more.
