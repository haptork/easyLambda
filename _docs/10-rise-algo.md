---
title: "Rise Algorithms"
permalink: /docs/rise-algo/
excerpt: "Rise function objects"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

EasyLambda provides commonly used generic function objects for the rise unit.
These objects provide a wide variety of configurability in the form of
properties specific to them. These properties are defined only for these
function objects and are not to be confused with the properties that apply to
units or dataflows.

All the function objects for rise are embellished with the 
`split` property which configures how the data generated / loaded by the
function object is to be shared among the processes. If it is set, the total
data is split among the processes otherwise each process gets a full copy of
the data.

Following are some of the common generic function objects for rise unit.
Check demoFromFile and demoIO for usage examples of these function objects.

## fromMem 
It loads rows from a container or a C++ initializer
  list. It expects a container and streams rows with multiple columns to the
dataflow. If the same list is apriori available to all the processes, splitting
the data parallelizes the operation. however, if the list is already split
amongst the processes i.e..  each process has different items in the list, then
\texttt{split} property is not required.

## fromFile
 It loads tabulated data from files in parallel. The
  types of the columns to be read into the tables can be specified using
  template parameters which can be either C++ primitives or std::array
  types. fromFile handles errors in reading and can read in a single
  file or multiple files simultaneously. 
  
  It has a comprehensive set of properties to configure. As one example, it can
  either impose a strict or a loose schema on the data read in. A strict schema
  ensures that the rows that have a different number of columns than specified
  are rejected. A loose schema ensures that extra columns get ignored and
  default values get filled in for rows that don't have values for certain
  columns. 
  
  If the data stored in the files is segmented on certain columns and a reduce
  with key as the same columns needs to be applied, it can be carried out
  in--process if it is made sure that the same process reads the data unless
  the value in the key columns change. The fromFile function object provides
  a segmented property with column selection for the same.

  It can also be used for reading in data with header information which needs
  to be attached to every row. This is a common requirement in physics
  simulation programs which dump out time--steps at the top of the files and
  the data rows for the time--step follow the header. The data loaded in such a
  manner is always segmented on the header column value.

## iota 
It can be thought of as a lazy range generator
function. It calls the next unit with incrementing numbers within a specified
range. The values of the generated range can be split among processes
controlled by the `split` property that is by default set.

## kick
It calls the next unit a specified number of times with
rows having no columns that translate to no parameter in the user function of
the next unit. The split property can be used to control whether
the number of calls will be total calls over all the processes or number of
calls for each process. The split property is by default set. 

## fileNames
The function object loads all the file paths from a glob pattern. The file names
can be shared among the processes with split property. It is useful in non text
files such as images. The function object loads the file paths which then are
passed to the next unit which can open the file and return the data.
