---
title: "Data Flow Expressions"
permalink: /docs/dataflow-expr/
excerpt: "Refernce on data flow expressions"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

The data-flow expressions can be added to any prior unit in a data-flow or to
any prior expression. They finalize the prior data-flow unit, that means if we
have added a map and then we call any data-flow expression like build, we now
can not change the properties of the map.

## build

  build expression returns the current unit which then might be used with
  [ezl::flow]({{ base_path }}/docs/starting-dataflow#ezl::flow),
  [addFlow](#addFlow) or [branchFlow](#branchFlow).

## run

  run expression runs the data-flow. If there is no rise then the data-flow
  run has no effect. Notwithstanding to on which unit run is called on, running
  the data-flow starts streaming to all the branches in a data-flow, starting
  from rise to the end units.

  Similar to build it returns the unit which then might be used with
  [ezl::flow]({{ base_path }}/docs/starting-dataflow#ezl::flow),
  [addFlow](#addflow) or [branchFlow](#branchflow).

## runResult

  It runs the data-flow similar to run, however it returns the output rows of
  the unit as a vector of tuple of column types.

## oneUp

  The oneUp expression makes the subsequent expressions applicable to one unit
  prior to the current one in the current data-flow. If another unit is added
  after oneUp then the current unit and the other unit both receive the data
  stream from the prior unit. However, there is no way to add further expressions
  to the current unit. It is particularly useful for dumping aggregate reductions
  or summary of intermediate results and continuing with the actual computation
  by adding the other units as shown in example [cod16]().

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
