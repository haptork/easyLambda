---
title: "Data Flow Properties and Expressions"
permalink: /docs/dataflow-expr/
excerpt: "Reference on data flow expressions"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

A linear dataflow in easyLambda can be constructed by simply adding units to the
dataflow with the dot operator (.). Non--linear dataflows that incorporate
branches, merges or cycles can be built using dataflow properties like
`pipe`, `merge`, `tee`, `oneUp` or explicit stream
operators like `<<`, `>>`, `+`. 

A dataflow can be built and assigned to a variable with the `build`
expression. A dataflow runs only when `run` or `get` is called on
it; `run` returns the dataflow same as build does while `get`
returns the resulting rows from the output stream of the dataflow. The resulting
rows are returned as a vector of tuples representing rows and columns. Once
built, a flow can be run multiple times with different data or can be added as
a component to construct larger dataflows. It is possible for a user function in
a dataflow to build or run another dataflow inside the function body.

## build

  build expression returns a `ezl::Flow` object.

## run

  run expression runs the dataflow. If there is no rise then the dataflow
  run has no effect. Notwithstanding to the unit run is called on, running
  the dataflow starts streaming to all the branches in a dataflow, starting
  from rise to the end units.

  Similar to build it returns the unit.

## get

  It runs the dataflow similar to run, however it returns the output rows of
  the unit it is called on as a vector of tuple of column types.

## pipe 
  The pipe property accepts a pre--built flow and pipes the
  output stream of current unit to it.

## merge
 It accepts a pre--built flow and merges its output stream
with the output stream of the current unit. Effectively, the next unit in the
flow receives a composite input stream comprising of both.

## tee
 It accepts a pre--built flow and pipes the output stream
of current unit to it, however new data--flow units are now added to the same unit
on which tee is called. The next unit and the data--flow passed as parameter
both receive the output stream of the current unit in the dataflow.

## oneUp
It is often useful to view the aggregated intermediate
results of a computation without interrupting its progress. Its possible to
achieve this by building multiple flows and connecting with a tee. However, the
oneUp property makes this frequent pattern very concise. It takes no argument.
The next unit added following the oneUp property and the unit oneUp is called
on, both receive the output stream of the just prior unit in the dataflow.
Effectively, the unit that calles oneUp becomes a tee branch in the main trunk
of the dataflow under construction.


## ezl::Flow
  This is the object that gets returned when a flow is built or run (actually
  shared ptr of the object which in future version is likely to change to just
  the object).  

  A dataflow can be characterized solely by its input and output row types. A
dataflow object can be conceptualized as a black box that takes one type of
rows and converts them into another type. A Flow object is returned with a
build or run expression. It is an independent dataflow which can be added as a
component of another dataflow. It can be passed around, run or attached to
another dataflow.

   {% highlight cpp %}
  auto fl = ezl::flow<char, int>()
              .map<2>([](int i) { return i * i; })
              .build();
  // fl is shared_ptr of type:
  // ezl::Flow<tuple<char, int>, tuple<char, int, int>>
  // The first tuple has cols of input stream and second has columns of output
  {% endhighlight %}


We need to explicitly mention the type and not use auto when splitting a
project code in header and source files. It is then becomes necessary to explicitly
mention the Flow object type if a function returns the result of build / run.


If the flow needs only to be used with ezl::flow(...) for adding units that
recieve the output rows from it then it can also be made of the type:
`ezl::Source<tuple<outputCol1, outputCol2, ...>>`. In this case we are
only mentioning output column types and making it a Source type, the
input column are irrelevent since we only intent to use it as Source to other
flows or units. Similarly, if we are to use a Flow with only tee, we
are only concerned with the type of rows it can take as input and we can
use the type `ezl::Dest<tuple<inputCol1, inputCol2, ...>>`. It is fine to
have shared_ptr of either of Flow, Source or Dest as required. Flow<I, O> is
derived from Source<I> and Dest<O>.

Useful methods / operators of the flow object:

- `<<` : input stream operator << can be used to add another flow / source
       such that the flow recieves rows from the other flow / source.
- `>>` : output stream operator >> can be used to add another flow / dest
       such that the flow sends streams the rows to the other flow / dest.
- `+`  : to merge two flows such that the resulting flow object input-stream
       and output-stream are combined streams of the two operand flows.
- `unlink` : remove all the links (sources / dests / flows) added so far to
           the flow.
