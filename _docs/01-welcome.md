---
title: "Welcome"
permalink: /docs/welcome/
excerpt: "Introduction to easyLambda, data-flow, map-reduce, MPI and modern C++."
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

Welcome to easyLambda and thanks for your interest. The site aims to be a
comprehensive guide for easyLambda such that a C / C++ programmer of any level
of experience can follow the material and get started using easyLambda.

## What is easyLambda

EasyLambda is a header only C++14 high-performance library for manipulating data 
streams in parallel with functional list operations (map, filter, reduce, scan, zip)
that are tied together in type-safe data-flow. It has declarative interface with
novel column selection for better composition of functions. It provides
MPI parallelism without any parallel details in user code and comes with generic 
algorithms for I/O and frequently used computations on data to make data-processing
with C++ easy and fast.

The parallelism is implicit by the semantics of data-flow and map-reduce
and can be controlled as a property of an operation declaratively. The
interface of the library is minimal and non-redundant. 

Programming with pure functional data-flow is similar to the way we think in
spreadsheet programs, SQL queries or declarative commands like gawk, 
cut etc, without any restrictions (you can apply any C/C++ function to data columns).  

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
resulting column(s) at the end of the row. The number in the
angular brackets specify column number(s) to work on. dataSource can be from 
a list variable, file or anything like networt etc because source can also be
junt another C / C++ function.

So, we get the declarative simplicity of spreadsheet programs or SQL inside C++
and we can use any C++ function to tranform the data. If the data size grows
then we can just run the same code in multiple cores or multiple nodes. The
syntax might look unfamiliar at first but there is a very minimal set of things
to understand before you begin writing code with easyLambda and this guide
tries to put these details together. In the process you will be introduced to
some concepts in modern C++ and functional programming that are helpful outside
as well.

### What is MPI

MPI is a message passing library for distributed parallelism. It is extensively
used in HPC for distributed parallel programs. It scales well for multiple
cores in single system as well as for thousands of processes across nodes of
a cluster. easyLambda uses MPI internally and user code does not need to make 
MPI calls directly.

The programs that use MPI are compiled with mpicc that is a compiler wrapper.
The compiler flags and everything else remains same. The executable can be run
as usual as a single MPI process or it can be run with mpirun command with multiple
process. mpirun -n 64 ./a.out runs the a.out with 64 processes. It is simple to
use and ezl programs can take full advantage of multiple cores or multiple
nodes on a cluster without any change in the program and data-flow.

### What are functional list operations (map, reduce etc.)

The map, filter, reduce etc are higher order functions
(i.e. they take a function as one of the input) defined in functional 
programmaing for list operations. Unlike for loops in imperitive programming
these functions may or may not specify an ordering in which the function
is applied on the list, making them more of a set operations. This property
is the basis of automatic data-parallelism.

Map takes a function and a list as parameters, applies the function to each
element of the list. This is equivalent of for_each loop, however map doesn't
state the order in which the function is applied to the items. Filter is similar
to map but takes a predicate (a
function that returns boolean) and a list as parameter. The only rows for which
the predicate returns true remain in the resulting list.

Reduce takes a function, a list and initial value of the result as parameter. 
The user function takes list
item and prior result value as input and returns an updated result. Again, the
result of the operation must not depend on the ordering of the list items.
Reduce operation is very often applied to each group of rows separately. For
e.g. finding number of atoms in each time-step instead of all the atoms in all
the time steps or finding the number of players in each team.

In easyLambda the operations are performed on streams that is, the
resulting row from one operation streams to the next operation without
buffering the whole list or waiting for next unless required. The 
streaming behavior is memory efficient and is better for working with
generators / infinite lists.

## Why easyLambda

Use ezl for your table / list-processing tasks, to write post-processors for
simulation results, for iterative machine learning algorithms, to write
parallel codes easily, to use its many generic functions that include parallel
type-safe reader, summary of data, correlation etc or to have fun with data-flow
programming in C++. easyLambda naturally encourages users to split the code in 
many small units that can be reused more with column selection and it incurs no
overhead compared to equivalent serial monolith code or bare MPI only code. This
is made possible by making minimal copies of data for passing between units, 
functions and column selection etc.

The library enforces no special structure, data-types or requirements on the
user functions. Moreover, it facilitates composition of functions with core
algorithm logic, irrespective of nearby columns using column selection. A
uniform interface with limited number of well defined list operations that
have common properties for parallelism, column selection, IO etc simplifies
programming and enables inherent parallelism.

easyLambda has various distinguishing features such as cyclic data--flows,
parallelism as a property, column selection for composition, parallel file
reading with key columns, flushing of reduce results based on ordering, reduce
on adjacent rows and implicit configuration using type--traits and
meta--programming.

If you are a C++ enthusiast or you like functional programming then there is
a high chance that you will find the project quite interesting. There are a
lot of interesing ways the metaprogramming is being used in easyLambda. 
Contributions and feedback of any kind are much appreciated. You can use github
issues or e-mail for any suggestions. Please fork the repsitory and
contribute, the library can be improved a tons. Check the blog section for
discussion on the challanges and exciting implementation details.

### Why C++14 in easyLambda

Writing a high-level generic library with compile time type-safety would have
been either impossible or extremely hackish if it were not written with C++14.
Moreover, without C++ lamdba functions, auto keyword and smart pointers the
user code would look terribly complicated. The library code stands upon the
type deductions, tuples and variadic templates of C++14. easyLambda deduces
types wherever possible, there is not a single redundant type declaration
for target type, source type, column selection or anything. Use of compile time
polymorphism and metaprogramming decreases runtime overheads as well.

Using C++ and enabling use of simple C functions is important for easyLambda as
these are the languages that are most commonly understood in scientific
community. For simulations, writing post-processors is a regular task but there
are no standard ways to do so easily without taking care of I/O, parallelism
and there is not much to facilitate reuse and sharing of algorithms. easyLambda 
might be the only library for C++ that provides solutions to these data-processing
needs.

### Why MPI in easyLambda

MPI is high performance message passing library that is extensively used in
HPC / scientific community and with advent of high performance data science,
it is being used in state of the art deep-learning frameworks. The questions that
remain are why it is less used outside HPC or why HPC doesn't use other
libraries. These questions have been discussed a lot of times without any clear
conclusion [link](http://www.dursi.ca/hpc-is-dying-and-mpi-is-killing-it/)
[link](http://www.csm.ornl.gov/workshops/SOS17/documents/Plimpton_sos_Mar13.pdf).
We hope that easyLambda will make using MPI easier in scientific community and
might make MPI ubiquitous even outside of the niche.

There are no libraries in MPI that provide an easy way of using MPI for reusing
existing serial algorithms. EasyLambda tries to fill this void. EasyLambda
programs are terse compared to MPI-only implementation with no to little effect
on performance.

### Why Data-flow and Map-Reduce in easyLambda

The impertive programming does not work well with the current parallel architectures.
In imperative programming we express the algorithms
in total order of the execution of instructions even when we don't want to, e.g. use
of loops for set operations or ordering two independent computations making them necesserily
execute one after another even if it won't change anything if they are
executed other way around. With data-flow, partial ordering of computation can be expressed with different
branches in a data-flow. Map and reduce operations express set operations without
implying any ordering. It is the implicit partial ordering that enables
parallelism if required. Data-flows with column selection also provide good
scope of reusing functions while compile time type safety ensures only well
formed data-flows are compiled. Data-flows can themselves be composed / nested.
