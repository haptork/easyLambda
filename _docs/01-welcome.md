---
title: "Welcome"
permalink: /docs/welcome/
excerpt: "Introduction to easyLambda, MPI and modern C++. "
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

Welcome to easyLambda and thanks for your interest. The site aims to be a
comprehensive guide for easyLambda such that a C / C++ programmer of any
level of experience can follow the material and get started using 

## What is easyLambda

EasyLambda is a single header C++14 library to write parallel type-safe
dataflows consisting of map and reduce operations primarily for
data-processing. It has declarative interface, good support for
column selection, MPI parallelism w/o any user code, generic algorithms for I/O
and general computations on data. Together these features provide easy and
expressive way to compose your C / C++ functions to carry out operations on
tabular / list data. The parallelism is implicit by the semantics of data-flow
and map-reduce and can be controlled as a property of an operation
declaratively.

Programming with pure functional data-flow is similar to the way we think in
spreadsheet programs or SQL queries, while being sequential and less
restrictive (you can apply any C/C++ function to data columns). 

Let's say we have data with many columns, we want to add first two columns and
find how many of them add up to more than a hundred. In spreadsheets one way of
doing this can be as follows:

- write formula A1 + B1 in the first row of a new column. 
- apply the same formula to all rows of the new column by auto-filling.
- filter for greater than 100.
- count the filtered rows.

Here, is the easyLambda code for the same.

{% highlight ruby %}
ezl::flow(dataSource)
  .map<1, 2>(std::plus)
  .filter<-1>(ezl::gt(100))
  .reduce(ezl::count(), 0).dump()
  .run();
{% endhighlight %}

The data flows to the functions wrapped inside units (map, filter, reduce) one
after another. The map step applies a C / C++ function to all rows and adds a
column with the result at the end of all the columns. The number in the
angular brackets specify column number(s) to work on, -1 column is for the
first column from the end. dataSource can be from a list variable, file or some
network as we will see later.

So, we get the declarative simplicity of spreadsheet programs or SQL inside C++
and we can use any C++ function to tranform the data. If the data size grows
then you can just run the same code in multiple cores or multiple nodes. The
syntax might look unfamiliar at first but there is a very minimal set of 
things to understand before you begin writing code with easyLambda and this
guide tries to put those details together.

### What is MPI

MPI is a message passing library for distributed parallelism. It is extensively
used in HPC for distributed parallel programs. It scales well for multiple
cores in single system to thousands of processes across nodes of a cluster.
easyLambda uses MPI internally and user code does not need to make MPI calls
directly. 

### What are Map and Reduce

The map and reduce are higher order functions in functional programming i.e.
they take a function as one of the input. 

Map takes a function and a list as parameter, applies the function to each
element of the list and outputs the transformed list. This is equivalent of
for_each in some languages, however there is a subtle difference, it doesn't
impose an order in which the list items should be transformed and this enables
implicit parallelism. In imperitive programming we always express a program as
instructions executing one after another, even if we don't care about ordering.
Filter in easyLambda is a similar operation but it is used to qualify if a
row passes to the next unit or not based on some criteria.

Reduce takes a function, a list and initial value of the result as parameter.
It then calls the function for each list item. The user function takes list
item and prior result value as input and returns an updated result. Again, the
result of the operation should not depend on the ordering of the list items.
Reduce operation is very often applied for a group of rows. For e.g. finding
count of students of each team or finding number of atoms in each time-step.
In addition to simple reduce, easyLambda provides options to have ordering in
reduce or have all the rows at the same time etc.

## Why easyLambda

Use ezl for your data-processing tasks, to write post-processors for simulation
results, for iterative machine learning algorithms, for general in-memory list
data processing, to write parallel codes easily, to use its many generic functions
that include parallel type-safe reader, summary, correlation etc or just to use
data-flow.

The library enforces no special structure, data-types or requirements on the
user functions. Moreover, it facilitates composition of functions with core
algorithm logic, irrespective of nearby columns using column selection. A
uniform interface with only map and reduce computations that have common
properties for parallelism, column selection, IO etc simplifies programming
and enables inherent parallelism.

easyLambda has various distinguishing features such as cyclic data--flows,
parallelism as a property, column selection for composition, parallel file
reading with key columns, flushing of reduce results based on ordering, reduce
on adjacent rows and implicit configuration using type--traits and
meta--programming.

If you are a C++ enthusiast then possibly you will find the project quite
interesting. Contributions and feedback of any kind are much appreciated.
Please form the repsitory and contribute, the library can be improved a tons.
Check the blog section for discussion on the challanges and exciting
implementation details.

### Why C++14 in easyLambda

Writing this high-level generic library with type-safety would have been either
impossible or extremely hackish if it were not written with C++14. Moreover,
without C++ lamdba functions, auto keyword and smart pointers the client code
would look terribly complicated. 

Using C++ and enabling use of simple C functions is important for easyLambda as
these are the languages that are most commonly understood in scientific
community. For simulations writing post-processors is a regular task but there
are no standard ways to do so easily without a worry about I/O, parallelism,
code reuse and composition etc. easyLambda might be the only library for C++
with data-flow, column selection and map-reduce.

If you are new to C++14, you only know C or you are not comfortable with those
angular brackets often used for column selection and column types in ezl then
check the [essential C++]() section.

### Why MPI in easyLambda

MPI is high performance message passing library which heavily lacks good
library support on top of it but still is used in HPC majorly. The questions
that remain are why not outside HPC and why not use other libraries in HPC.
These questions have been discussed a lot of times without any clear conclusion.
[link] [link]. We hope that easyLambda will make using MPI easier in scientific
community and might make MPI more visible outside the niche community.

Another amazement is that there are no libraries in MPI that provide an easy
way of using MPI for the existing serial algorithms. EasyLambda tries to fill this void.

### Why Data-flow and Map-Reduce in easyLambda

Map-Reduce with type-safe data-flow has proved to be very effective for
data-processing tasks in general (e.g. summingbird). Data-flows are also
being effectively used for task parallelism with promises in nodejs libraries.

easyLambda adds its own things to it like ExpressionBuilder interface for
configuring parallelism, output columns, dumping resuts etc, column selection,
order based flushing in reduce, reduceAll units, cyclic data-flows etc.

Apart from being highly effective for tasks in hand, it feels right to be able
to express partial ordering with different branches in a data-flow or with map
function rather than a loop from first element to last. It is this partial
ordering that enables parallelism but even if we are running in a single node
it is better to express the computation as is. Imperitive programming makes 
computations highly ordered in execution and in our way of thinking. Data-flows
with column selection also provide good scope of reusing functions that have no
constraints other than type-safety.  These form a basic computing blocks which
themselves can be composed / nested.
