1 Data-flow
============

Programs in easyLambda (ezl) are written as a flow of data from one computation
unit to another. A data-flow program is a composition of various independent
functions each one doing some small task. 

Let's look at an ezl program to find the summation of a number with its square
root.

```cpp
ezl::rise(ezl::fromMem({25.0}))
  .map(std::sqrt)
  .map(std::plus).dump()
  .run();
```

There are three units in the above program. It starts with `rise` which takes a
function that is source of the data. `fromMem` is an ezl function
to get the input data to the flow. Next are two `map` units, the number flows
from rise to map-sqrt, the input number and the result of `sqrt` (25.0, 5.0) is
passed to map-plus. The data that comes out of second map (25.0, 5.0, 30.0) is
printed on to the standard output using `dump`. The flow runs only when `run`
is called. In addition to running a flow, it can also be built and run later
multiple times with different data or can be added to another data-flows.
`dump()` can be added to any unit as a property of that unit, similar
embellishments exist for parallelism and for compositions as we will see in
further sections.

The data-flow paradigm is declarative, unlike general C++ procedural programs it
is more expressive. The examples of declarative paradigm include computations
in spreadsheet programs like MS-excel, SQL queries etc. Data-flow is prevalent
in functional programming paradigm. Now that more and more languages are welcoming
functional paradigm, good data-flow syntax can be seen in nodejs promises
such as bluebird, angularjs, sequelize etc., many Scala codes and libraries.
What this all means is that an ezl program can be thought, read and written in
terms of operations that we are performing on data. To write a program in ezl,
one writes the algorithm logic (or reuses existing ones) in terms of (pure)
functions (returning result using immutable inputs), uses them in the flow,
takes care of I/O, data-format, parallelism etc by modifying properties of the
units. The units can be added or omitted from a flow, or a flow can be added
as a branch of another flow or even to itself in circular fashion. We will see
details of these things in the sections that follow.

> Related works: There are some libraries that offer data-flow in C++ such as
> boost data-flow, tbb graphs, phish. Unlike data-flows in functional paradigm they
> do not feel natural because of the bloated syntax to explicitly form graphs,
> edges, nodes and sometimes different sort of classes, one ends up writing
> functions or classes in unusual ways to use them in the data-flow. On the other
> hand functional languages try to facilitate composition of functions that have
> nothing but algorithm logic to ease reuse, modularity and parallelism without
> bloat code. ezl follows the same path. The functions in a data-flow have no
> dependency on ezl and require no extra construct. Just take multiple input
> parameters as usual and return the result. Moreover, there is a lot of emphasis
> on column selection to use functions in the data-flow that do not exactly match
> the inputs coming. There is no other C++ library that provides map, reduce
> with data-flow. In addition ezl provides MPI parallelism which notwithstanding
> to its extensive use in HPC, lacks good library support written on top of it.

#### Composition in ezl data-flow

Data-flow involves composition of functions each one of which does a small
task and returns output for further processing if needed. The data-flow of ezl
is type-safe at compile time i.e. the data types of composition (parameters
of a unit with resulting types of prior unit) should match at compile time.

A lot of times parameters of a function are (proper) subset of parameters being
passed to it from prior unit, hence a composition is not directly possible.  In
functional languages composition of such functions is one of the motivation for
the use of [Monad](https://en.wikipedia.org/wiki/Monad_(functional_programming)) idiom. 
ezl has columns / parameters selection for these cases.

Let's look at an ezl program to find the sixth root of a number.

```cpp
ezl::rise(ezl::fromMem({number}))
  .map(std::sqrt).colsResult()        // with colsResult() only result is passed
  .map(std::cbrt).colsResult().dump()
  .run();
```

The `colsResult()` is a property of `map` unit. It makes only the result pass
to the next unit. The program prints the sixth root. We can select output
columns by their indices as well. In place of `colsResult()` we could have
written `cols<2>()` or `colsDrop<1>()`. The `cols<...>()` and `colsDrop<...>()`
properties are equally applicable to other computing units that we will discuss
later.

If we use `colsResult` only with first map and not with second one then the
program will print the second root (input) and sixth root (output) both. But
what if we want to print number and the sixth root. Following is the program
for it.

```cpp
ezl::rise(ezl::fromMem({number}))
  .map(std::sqrt)
  .map<2>(std::cbrt).colsTransform().dump() // map<2> to pass only 2nd col
  .run();
```

The second column is selected for the input to `cbrt` with map<2>. Obviously, we
can select multiple columns, in any case the default output from the unit
remains to be all input columns followed by output columns. The
`colsTransform()` is a property of `map` that replaces the selected input
columns of the map (here 2nd) by the output cols returned by the function.
Without `colsTransform()` this will print number, sqrt, sixth root. Here,
`colsTransform()` can be replaced by `cols<1, 3>()` or `colsDrop<2>`.

> Summary: The default output from a map property is all the inputs followed by
> all the output columns. The columns for output can be selected using
> `cols` or `colsDrop` properties with indices of columns. There are two
> syntactic-sugars for `map` unit: `colsResult` and `colsTransform`.
>
> The column selection can be applied before passing the input to a `map` unit
> (in general applicable to other computing units as we will see later). This
> is useful if the map user function works on a subset of columns / parameters
> while other columns need to be forwarded in the pipeline.

#### More on data-flow

This sub-section is for the things that we mentioned or used in the above
examples but didn't describe and for a few additional data-flow features that
are not often used.

##### Returning multiple values / columns:

A function can return multiple values / columns by returning a standard tuple.
The function that returns square root and a text message can be written as.

```cpp
auto mysqrt(int number) {
  return std::make_tuple(std::sqrt(number), "sqrt calculated");
}
```

##### Branches

There are different ways of creating a branched data-flow. We will demonstrate
this by creating a data-flow with two different maps, one for finding sqrt and
one for cbrt both of which get data from a common source.

```
|      | --> |sqrt|-[dump]
|number|
|      | --> |cbrt|-[dump]
```

Many different variations for this data-flow are given below.

```cpp
ezl::rise(ezl::fromMem({number}))
    .map(std::sqrt).dump()
      .oneUp()            // continue data-flow from one unit backwards / up
  .map(std::cbrt).dump()
  .run();
```

`oneUp()` is good for adding a single unit as branch in the data-flow and then
continue with building the main data-flow from the prior unit. Here, we add a `map`
to the `rise`, go one-up to the `rise` again and add another `map` to it.

```cpp
auto srcFlow = ezl::rise(ezl::fromMem({number})).build();

auto sqrtFlow = ezl::flow(srcFlow)
                  .map(std::sqrt).dump()
                  .build();

auto cbrtFlow = ezl::flow(srcFlow)
                  .map(std::cbrt).dump()
                  .build();

cbrtFlow.run();
```

This is a neat way of writing data-flow when there is more than one long
branch. We build single units at a time, adding them to existing units using
`flow(...)`. Here, we add sqrtFlow and cbrtFlow to srcFlow and then run the
data-flow. Running either of the three flows will have same effect. The data
flows from `rise` to every unit attached to it irrespective of which one called
the `run()`.

```cpp
auto sqrtFlow = ezl::flow<int>().map(std::sqrt).dump().build();

auto cbrtFlow = ezl::flow<int>().map(std::cbrt).dump().build();

ezl::rise(ezl::fromMem({number}))
  .branchFlow(sqrtFlow)
  .addFlow(cbrtFlow)
  .run();
```

We have created two flows without adding any source to them but specifying the
type of the source, for the flow viz. one column of type `int`. We have added a
branch with `branchFlow` and then continued adding to the main flow we were
building before adding the branch. `addFlow` adds a flow in the main flow
instead and not as branch. Since, we are not adding anything after `cbrtFlow`
we might as well use `branchFlow` instead of `addFlow` in this case.


```cpp
ezl::rise(ezl::fromMem({number}))
  .branchFlow(
    ezl::flow<int>().map(std::sqrt).dump().build()
  )
  .map(std::cbrt).dump()
  .run();
```

If we are not reusing a flow somewhere else then we can build it inside
`branchFlow` as well.

##### Cycles

We have implicitly seen that `map` unit transforms a value to another
value. There is also a `filter` unit that passes the input values only
if a predicate / condition on the values is satisfied. The column
selection for input to the function in the `filter` unit and for output
from it using `cols<...>()` is same as for a map unit. Using `filter`
we can create a cyclic data-flow same as the first data-flow of the
following figure.

![dataflow](../dataflow.png)

Following are the two ways of creating the data-flow.

```cpp
auto srcFlow = ezl::rise(ezl::fromMem({number})).build();

auto sqrFlow = ezl::flow(srcFlow).map([](double number) {
                 return number * number;
               }).colsResult().build();

ezl::flow(sqrFlow)
  .filter(ezl::gt(100)).dump()  // value only passes if value > 100
  .build();

ezl::flow(sqrFlow)
  .filter<2>(ezl::lt(100))      // value only passes if value < 100
  .addFlow(sqrFlow)  // cycle
  .run();
```

```cpp
auto sqrFlow = ezl::rise(ezl::fromMem({number}))
               .map([](double number) {
                 return number * number;
               }).build();

ezl::flow(sqrFlow)
    .filter(ezl::gt(100)).dump()
    .oneUp()
  .filter<2>(ezl::lt(100))
  .addFlow(sqrFlow)  // cycle
  .run();
```

##### More variations

In addition to branches and cycles, a data-flow can be constructed and run inside
another data-flow. This cascading can be any number of times. Moreover, two-branches
of a data-flow or different data-flows (data-flows with different rises) can be
merged into a single flow. A data-flow can have any number of rises as well. So,
the only restriction for data flow from one unit to another is that the types
should match. Check [demoFlow](../../examples/demoFlow.cpp) for more crazy example
flows.

##### Getting result

`dump()` can be added to any unit to display its output data. One can think
of it as a branch in the main data-flow. `dump` has two optional string parameters
viz. file-name and header. An empty file-name prints the data to the screen. A
non empty header string is printed at the top of the file.

To get the end result of the data-flow as a return value one can use
`.runResult()` in place of `.run()`. The return value is always a vector of
tuple of various values returned.

Following is a function that returns sixth root of a number passed to it.

```cpp
auto sixthRoot(double number) {
  auto res = 0.0;
  std::tie(res) = ezl::rise(ezl::fromMem({number}))
                    .map(std::sqrt).colsResult()
                    .map(std::cbrt).colsResult()
                    .runResult()[0];
  return res;
}
```

##### Multiple rows

We have restricted our data-flow examples to the calculation for a single number.
However, we can pass a list (array, vector or other containers) of numbers in the rise and
the same data-flows will transform each number / row in the list. This
is the usual semantics of `map` in functional paradigm i.e. `map` transforms
a list into a new list by applying a function to each element of the list.

Following are some of the different rises that can replace our previous
rise.

```cpp
auto srcFlow = ezl::rise(ezl::fromMem({5.0, 19.4, 43.5})).build();

auto ar = std::array<double, 3> {{5.0, 19.4, 43.5}};

auto srcFlowAr = ezl::rise(ezl::fromMem(ar)).build();

auto vec = std::vector<double> {5.0, 19.4, 43.5};

auto srcFlowVec = ezl::rise(ezl::fromMem(vec)).build();
```

We can also load numbers from a file directly.

```cpp
auto fileName = std::string("numbers.txt");

auto srcFlowFile = ezl::rise(ezl::fromFile<double>(fileName)).build();
```

The rise function `fromFile` can work with multi-column files as well. It is
quite comprehensive. It takes care of errors in reading, it has option to have
strictSchema (reject rows that have different size of column) or noStrictSchema
(fill in defaults if less cols, or ignore if more cols), parallel reading etc.
Check [demoFromFile](../../examples/demoFromFile.cpp) for more on these options.

In some cases it may be required to return zero, one or multiple rows for an
input row from a map. For this, the map can return a vector of values which
is treated as returning multiple rows. For returning vector as a column, a
tuple of vector is to be returned.


##### A peek at parallelism

A data-flow enables task parallelism, inherently. Any unit can be made to
run in a separate process / thread. A data-flow pipeline to calculate
sixth root can be thought as running in two different processes, one finds
sqrt and passes the result to next process to find cuberoot. This can repeat
for a list of numbers in parallel i.e. sqrt and cbrt can be working simultaneously
on different numbers. Even rise and dump can be in task parallelism in the data-flow.

The `map` enables data parallelism, inherently. A single unit let us say sqrt can be
made to run on separate processes / threads on different data simultaneously. So, let
us say we have a list of a hundred items, we can run the data-flow on two machines each
one calculates the result for its share of fifty numbers.

In practical applications task parallelism is beneficial if the computation tasks are
heavy or coarse and not all the operations require just the single row of a list for
their computations. For such computations we have `reduce` units, which are again
parallel but to a lesser extent.

The parallelism in ezl is currently implemented using MPI. It is a message-passing
library and can be scaled to a large cluster of computers very well. We will
discuss more on parallelism in later sections.

---------------------------------------------------------------------------------------

The [next section](2_mapreduce.md) introduces various details about map, filter and reduce
units.
