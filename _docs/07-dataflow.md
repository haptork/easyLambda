---
title: "Data Flow Expressions"
permalink: /docs/dataflow-expr/
excerpt: "Reference on data flow expressions"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

The data-flow expressions can be added to any prior unit in a data-flow or to
any prior expression. They finalize the prior data-flow unit, that means if we
have added a map and then we call any data-flow expression like build, we now
can not change the properties of the map.

## build

  build expression returns a `ezl::Flow` object which then might be used with
  [ezl::flow]({{ base_path }}/docs/starting-dataflow#ezl::flow),
  [addFlow](#addFlow) or [branchFlow](#branchFlow).

## run

  run expression runs the data-flow. If there is no rise then the data-flow
  run has no effect. Notwithstanding to the unit run is called on, running
  the data-flow starts streaming to all the branches in a data-flow, starting
  from rise to the end units.

  Similar to build it returns the unit which then might be used with
  [ezl::flow]({{ base_path }}/docs/starting-dataflow#ezl::flow),
  [addFlow](#addflow) or [branchFlow](#branchflow).

## runResult

  It runs the data-flow similar to run, however it returns the output rows of
  the unit as a vector of tuple of column types.

## oneUp

  The oneUp expression makes the subsequent data-flow expressions applicable to one unit
  prior to the current one. If another unit is added
  after oneUp then the current unit and the other unit both receive the data
  stream from the prior unit. It is particularly useful for dumping aggregate reductions
  or summary of intermediate results and continuing with the actual computation
  by adding the other units as shown in example [cod16]() or [logreg]().

## branchFlow

  It adds a data-flow unit (returned from build or run) as a branch destination
  to the current unit. After the branchFlow expression, the trunk data-flow
  continues from the current unit as usual i.e. any data-flow, unit expression
  or dump property can be added to it.

## addFlow

  Similar, to branchFlow another data-flow is added as the destination to the
  current unit, however it is added to the trunk in the sense that the
  further addition of expressions is on the unit that is just added with the
  addFlow.

## ezl::Flow
  This is the object that gets returned when a flow is built or run (actually
  shared ptr of the object).

   {% highlight cpp %}
  auto fl = ezl::flow<char, int>()
              .map<2>([](int i) { return i * i; })
              .build();
  // fl is shared_ptr of type:
  // ezl::Flow<tuple<char, int>, tuple<char, int, int>>
  // The first tuple has cols of input stream and second has columns of output
  {% endhighlight %}

  There can be a function that returns the result of build / run expression. We
  need to explicitly mention the type and not use auto when splitting a project 
  code in header and source files and a function returns the result of 
  build / run.

  If the flow needs only to be used with ezl::flow(...) for adding units that
  recieve the output rows from it then it can also be made of the type:
  `ezl::Source<tuple<outputCol1, outputCol2, ...>>`. In this case we are
  only mentioning output column types and making it a Source type, the
  input column are irrelevent since we only intent to use it as Source to other
  flows or units. Similarly, if we are to use a Flow with only branchFlow, we
  are only concerned with the type of rows it can take as input and we can
  use the type `ezl::Dest<tuple<inputCol1, inputCol2, ...>>. It is fine to
  have shared_ptr of either of Flow, Source or Dest as required. Flow<I, O> is
  derived from Source<I> and Dest<O>.

  Useful methods / operators of the shared_ptr of flow object returned:
    - << : input stream operator << can be used to add another flow / source
           such that the flow recieves rows from the other flow / source.
    - >> : output stream operator >> can be used to add another flow / dest
           such that the flow sends streams the rows to the other flow / dest.
    - unlink : remove all the links (sources / dests / flows) added so far to
               the flow.

