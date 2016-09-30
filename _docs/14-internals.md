---
title: "Internals"
permalink: /docs/internals/
excerpt: "Internal Design and Implementation"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

## Design Overview
EasyLambda is a header only library compatible with C++14 compliant compilers.
The library code can be seen as three layers of abstraction and a helper module
that includes metaprogramming functions and type-traits. In addition there
are generic algorithms for maps, filters and reduces. The algorithms are
function objects and have no dependency on library and vice-verse.

<figure>
  <img style="max-width:450px; max-height:250px;" src="{{ site.url }}{{ site.baseurl }}/images/layers.png" alt="benchelastic">
  <figcaption>
    The three layers and helper modules of easyLambda library.
  </figcaption>
</figure>

The first layer consists of interfaces and template classes for dataflow
such as Source, Destination, Link, Task, Flow, Root etc that provide 
functionality for type-safe connections in a dataflow. The Source of template
type \verb|T| can pass data to destination of type \verb|T| only and
vice-verse.

The second layer builds upon the various classes of first layer and add behaviour
for map, filter, reduce, rise etc units over them. The classes use tuples as
the basic types and add column aware functionality from the helper meta module.
The third layer has expressions for specific properties for each unit,
dataflow, dump, parallelism.

## Immutability and Performance
Wherever possible, an attempt is made to minimize copy and move operations on
the data. The data returned by a user function in a unit is represented as an
tuple of immutable (const) references before it is streamed to some other unit
in the dataflow. This has the desirable property of ensuring that if two units
receive the output stream then they it remains same for both regardless of
which unit operates on the data first. Creating a tuple of immutable references
is a cheap operation because it involves no copies. 

The default output for `map` involves concatenation of the input and
user function results.  Similarly, for the reduce, it involves concatenation of
the keys and user function results. This concatenation is implemented by
creating a tuple of immutable references. With column selection, the final
tuple has the immutable references of columns that are specified by the user.
Hence, the column selection has no extra performance penalty.

## Dataflow Representation

A dataflow can be characterized solely by its input and output row types.
A dataflow object can be conceptualized as a black box that takes one type of
rows and converts them into another type. The abstraction of a dataflow that
accepts data rows with columns of type `Ts` and streams out rows with
columns of type `Us` is represented in easyLambda with the type
\verb|Flow<<tuple<Ts...>, tuple<Us...>>|. Concretely, a dataflow that accepts
rows with column types int and char and outputs rows with a bool column would
have type \verb|Flow<<tuple<int, char>, tuple<bool>>|. This type is derived
from the types `Dest<Us...>` that separately
represent the input and output streams. 

A Flow object is returned with a build or run expression. It is an independent
dataflow which can be added as a component of another dataflow. It can be
passed around, run or attached to another dataflow.

## Parallelism
The Manager is a Singleton class in helper module that allocates a parallel
allocation object to rise and to different parallel units based on the
availability, usage and request made by them just before running a dataflow.
The parallel request and parallel allocation classes are part of helper module.

The second layer contains Rise class which is derived from Source and Task.
The second layer contains another class called MPIBridge that is a Link (
Source as well as Destination) and Task type. All the tasks in a dataflow are
assigned a parallel allocation object before running the dataflow based on the
parallel request made by the user and processes available.

MPIBridge is placed in between the Source and Destination units if destination
is not an in-process unit. The prll property in third layer makes sure that
MPIBridge is inserted between the current unit and its source. The source unit
instead of passing directly to the current unit, passes the data to the
MPIBridge and the current unit receives from it. The send and receive ends of
MPIBridge run on different processes. MPIBridge abstracts all the details of
MPI parallelism from other classes. The communication is asynchronous.
Initially, some rows are sent as they stream in, which can be sent eagerly
depending on the MPI configuration. To avoid eager buffer overflow and avoid
cost of sending small messages, the subsequent messages are sent in big vector
buffers. The boost::mpi library is used to work at a higher level interface
compliant with C++. There can be similar classes to MPIBridge with different
parallel policies e.g. MPI RMA that can define their own expression and
building behavior in the third layer totally oblivious to any other unit or
class.

## Metaprogramming helper functions
The helpers include a number of meta-programming functions and structures that
are mainly responsible for column selection, providing uniform interface for
different type of function calls, type-traits to calculate output types from
input types, handle user function results in uniform way etc. The module
abstracts all the metaprogramming details such that all other classes in second
and third layer can cleanly implement functionality at higher level. Very clear
static errors are reported for compile time inconsistencies for e.g.
type-mismatches, index out of bounds in column selection, wrong user function
parameters etc. However, C++ template errors can still get very huge and scary
which is likely to improve with C++ template concepts that are expected to be
added in C++20 standards.

## Interface
The third layer has expressions for specific properties of various units,
dataflow, parallelism etc. These expressions are implemented in separate
classes. The layer also contains builder classes for the units. The builder
classes for units inherit from the expressions they want to use e.g. the map
builder inherits from dump expression class, parallel expression class and dataflow
expression class. The inheritance uses CRTP (as given in section
\ref{genbasic}) so that the expressions from base classes (such as a dump
expression) can return an object of the derived type (such as a map builder).
The expressions like \verb|cols<...>()| that change the template parameters
return an object with the same properties but of a template instance of
different type variables.
