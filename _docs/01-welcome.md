---
title: "Welcome"
permalink: /docs/welcome/
excerpt: "Introduction to easyLambda"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

Welcome to easyLambda and thanks for your interest. The site aims to be a
comprehensive guide for easyLambda such that a C / C++ programmer of any level
of experience can follow it to get started using easyLambda.

## What is easyLambda

EasyLambda is a header only C++14 library for data processing in parallel with
functional list operations (map, filter, reduce, scan, zip) that are tied
together in type--safe dataflow. It has declarative interface with novel column
selection for better composition of functions. It provides MPI parallelism
without any parallel details in user code and comes with generic algorithms for
I/O and frequently used computations on data to make data-processing with C++
easy and fast.

The parallelism is implicit by the semantics of dataflow and map-reduce and can
be controlled as a property of an operation declaratively. The interface of the
library is minimal and non-redundant. 

Programming with pure functional dataflow is similar to the way we think in
spreadsheet programs, SQL queries or declarative commands like awk, cut etc,
without any restrictions (you can apply any C/C++ function to data columns). 

Let's say we have data with ten columns, we want to add first two columns and
find how many of them add up to more than a hundred. In spreadsheets one way of
doing this can be as follows:

- write formula A1 + B1 in the first row of a new column. 
- apply the same formula to all rows of the new column by auto-filling.
- filter the rows for greater than 100 value of last / 11th column.
- count the filtered rows.

Here, is the easyLambda code for the same.

{% highlight ruby %}
ezl::flow(dataSource)
  .map<1, 2>(std::plus)
  .filter<11>(ezl::gt(100))
  .reduce(ezl::count(), 0).dump()
  .run();
{% endhighlight %}

The data flows to the functions wrapped inside units (map, filter, reduce) one
after another. The map step applies a C / C++ function to all rows and adds the
resulting column(s) at the end of the row. The number in the angular brackets
specify column number(s) to work on. The dataSource can be from a list
variable, file or anything like network etc. because source can also be just
another C / C++ function.

So, we get the declarative simplicity of spreadsheet programs or SQL inside C++
and we can use any C++ function to transform the data. If the data size grows
then we can just run the same code in multiple cores or multiple nodes. The
syntax might look unfamiliar at first but there is a very minimal set of things
to understand before you begin writing code with easyLambda and this guide
tries to put these details together. In the process you will be introduced to
some concepts in modern C++ and functional programming that are helpful outside
as well.

## Why easyLambda

Use ezl for your table / list-processing tasks, to write post-processors for
simulation results, for iterative machine learning algorithms, to write
parallel multi-core or distributed codes easily, to use its many generic
functions that include parallel type safe reader, summary of data, correlation
etc or to have fun with dataflow programming in C++. 

The following chart demonstrates the performance of easyLambda for HPC cluster
running with NFSv3. We compared the performance of Monte Carlo pi against bare
MPI and found it similar. There are benchmarks on more number of processes and
more problems that you can find along with the example source files in the
repository.

![Parallel expressions]({{ site.url }}{{ site.baseurl }}/images/benchhpc.png)


The following chart compares the spark and ezl performance on certain problems
on aws ec2 m3.2xlarge machines. The easyLambda was deployed with starcluster NFS3.

![Parallel expressions]({{ site.url }}{{ site.baseurl }}/images/benchelastic.png)

The following chart demonstrates the ease of programming with easyLambda. It can
be argued that how good lines of code represent the ease of programming but this
factor is a decent indicator especially when the difference is considerable. The
source code for other comparing works is taken from the examples whenever
available.

![Parallel expressions]({{ site.url }}{{ site.baseurl }}/images/benchloc.png)

Some easyLambda features that matter: 

- provides a high level programming model over MPI which is way efficient than
  other parallel solutions but severly lacks library support.
  [[1]](http://www.sciencedirect.com/science/article/pii/S1877050915017895)
  [[2]](http://www.dursi.ca/hpc-is-dying-and-mpi-is-killing-it/).
- The programming model naturally encourages users to split the code in many
  small units that can be reused more with column selection. 
- It incurs no overhead compared to equivalent serial monolith code or bare MPI
only code. This is made possible by making minimal copies of data for passing
between units, functions and column selection etc.
- The library enforces no special structure, data-types or requirements on the
user functions for better reuse and interoperability with other libraries.
- Uniform interface with limited number of well defined list operations that
have common properties for parallelism, column selection, IO etc simplifies
programming.

### Contributions

If you are a C++ enthusiast or you like functional programming then there is a
high chance that you will find the project quite interesting. There are a lot
of interesting ways the metaprogramming is being used in easyLambda. It is a
new library with a tons of scope to improve from small bugfixes to big features
that can potentially change fundamental ways of parallel programming.
Contributions and feedback of any kind are much appreciated. You can use github
issues or e-mail for any suggestions. Please fork the repository and
contribute. Check the blog section for discussion on the challenges and
exciting implementation details.

### Why Dataflow and Functional

The imperative programming does not work well with the current parallel
architectures. In imperative programming we express the algorithms in total
order of the execution of instructions even when we don't want to, e.g. use of
loops for set operations or ordering two independent computations making them
necessarily execute one after another even if it won't change anything if they
are executed other way around. With dataflow, partial ordering of computation
can be expressed with different branches in a data-flow. Functional list operations
operations express set operations without implying any ordering. Dataflows with
column selection also provide good scope of reusing functions while compile
time type safety ensures only well formed dataflows are compiled.
