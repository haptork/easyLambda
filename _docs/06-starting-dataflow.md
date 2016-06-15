---
title: "Starting A Data Flow"
permalink: /docs/starting-dataflow/
excerpt: "Refernce on ways to start a data-flow"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

## ezl::rise

  ezl::rise is the original data source in a data-flow. It is a higher order
  function i.e. it takes another function as input that is called till it
  signals EndOfData. The function returns {row, isEndOfData} or a vector of
  rows in which case EndOfData is implied by returning an empty vector. Check
  the [code
  example](https://github.com/haptork/easyLambda/blob/e496a3e3070b806e8c48124d3454543c4cebc9b7/examples/demoRise.cpp)
  for more. The rise function streams the rows across all the branches of the
  data-flow when the data-flow runs. It doesn't buffer the rows.

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

  If a data-flow is not being started with a rise then it can either be started
  as continuation of another data-flow or it can be started by declaring the
  data-types of its input parameters without any source. A data-flow that has no
  source does not do anything when run. 

  - Properties (only if continuing from prior flow):
    1. [dump]({{ base_path }}/docs/dump-expr/)

  {% highlight cpp %}
  auto fl = ezl::flow<char, int>()
              .map<2>([](int i) { return i * i; })
              .build();

  ezl::flow(fl).dump()
  .filter([](char, int, int) { 
    return true; 
  })
  .run(); // doesn't do anything as there isn't a rise yet
  {% endhighlight %}

  Check the [code
  example](https://github.com/haptork/easyLambda/blob/e496a3e3070b806e8c48124d3454543c4cebc9b7/examples/demoFlow.cpp)
  for more.
