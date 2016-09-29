---
title: "Unit Expressions and Properties"
permalink: /docs/unit-expr/
excerpt: "Refernce on unit properties"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

The input and output types for a unit are fixed. A unit may output zero to many
rows of some column types. A function can return multiple values / columns by
returning a standard tuple. A vector can be returned for returning multiple
rows. A vector of tuple can be returned for returning multiple rows with
multiple columns. 

The output row from a unit is usually input columns (key columns in case of reduce)
followed by output columns. However, the output columns can be selected by their
indices with **cols** property e.g. using `cols<4,2>()` the output rows will consist
of 4th column followed by 2nd. Similary to cols property, there is **colsDrop** 
property, which instead of selecting the columns passed to it, drops them and passes
all the rest. If the default row has lesser number of columns than specified as
one of the column selection index, a static assert is raised at compile time.
If the user function returns multiple rows then the cols property is applied to
all the rows independently.

Similar to column selection for output rows, the input columns can also be
selected e.g. `map<3, 2>` selects the third and second column to be passed to
the user function. However, the input column selection for reduce selects the
key as discussed in the specific section on reduce below. 

The input parameters to the function can be column value types, constant references
or the tuple of value or constant references e.g. if 3rd and 2nd column are
int and float respectively, the function for map<3,2> can be fn(int, float),
fn(const int&, float int&), fn(tuple<int, float>), fn(const tuple<const int&,
const float&>&). All are equivalent, however constant references or tuple of
those do not make a copy and can improve the performance. Internally, there
are no copies made for column selection as the rows are represented internally
by tuple of const references. The parameters for reduce function are similar
but not exactly same and are described in the specific sections below.

## map

Adds map unit to the data flow. The map applies the input function to every
row as it streams from the input unit and passes the returned row to the next
unit. 

The column selection for input and output helps in reuse of the functions.
Certain column indices can be selected for the input to the user function passed
to map in any order e.g. `map<3,2>` selects the third and second column to be
passed to the user function.

The default output columns from a map is input columns followed by the columns
returned by the user function that can be changed using cols property. In addition
to the cols property there is colsTransform that replaces the selected input colums
with the function result columns and keeps the result of the columns as is in the
output row. With colsResult property only the function result columns are selected
for the output, dropping all the input columns.

  - Properties:
    1. [prll]({{ base_path }}/docs/prll-expr/)
    2. dump
    3. cols
    4. colsDrop
    5. colsTransform
    6. colsResult

The map processes a row as it is streamed from the prior unit and passes it to
all the next units without any buffering. When running in parallel, each map
runs inprocess by default and receives the rows from the prior unit in the same
process, without affecting the order of the rows. The prll property can be used
to change this behaviour and map running in different specified number of
processes can share the rows in round-robin fashion, can all recieve all the
rows or can receive rows partitioned on a input column. Check prll property for
more on this.

## filter

The filter requires predicate (function returning bool) based on which it decides
if the row is passed to the next unit or not.

  - Properties:
    1. [prll]({{ base_path }}/docs/prll-expr/)
    2. dump
    3. cols
    4. colsDrop

The filter has input selection similar to map which allows for reuse of generic
function objects very often.  Similar to map it streams the rows as they come
and runs inprocess by default that can be changed with prll property.

## reduce

The reduce takes a function, a list and an initial value
of the result. The user function passed to it is called for every new input row
with the current value of the result and the row. The result value is updated
with the return value of the user function. It is generally used for list
aggregation operations such as finding sum of all elements.

The reduce makes no guarantees about the order in which the function is to be
called over the list items for updating the result. Hence, it can only be
applied to lists that are associative under the given function and have an
identity result value.

The reduce, scan and zip operations can have keys that partition the input rows
into sublists to calculate the result for each key--defined sublist or group
separately. A sublist of rows is generally characterized by all the rows having
the same value of key columns where key columns consist of a subset of the
total columns. In SQL query syntax, reduce with partitioning is similar to
aggregate function with `group by`, while a zip with partitioning is
similar to a `join`. Reduce with partitioning is a frequently useful
operation, e.g. in computational physics, it is often necessary to calculate
properties for each time--step separately. E.g. reduce<3, 1> selects third and
first column as the key. All the non-key columns are value columns by default.
However with key and value both can be selected e.g. reduce<ezl::key<3,1>, ezl::val<4>>. 
The input parameters of the function are result columns, followed by key and
value columns that can be types, const refs or the tuple of key, tuple of value
followed by tuple of result or tuple of const refs e.g. if 1st, 2nd and 3rd
columns are char, int and float and result is of type bool, the function for
reduce<3,1> can be any of the following:

- fn(bool, float, char, int),
- fn(const bool&, const float&, const char&, const int&), 
- fn(tuple<bool>, tuple<float, char>, tuple<int>)
- fn(const tuple<const bool&>&, const tuple<const float&, const char&>&, const tuple<const int&>&)


The scan property that is similar to functional scan operation can be used to find
results like running sum etc. For each input row reduce updates the result as well
as passes the output row(s) from the result to the next destination units.

  - Properties:
    1. [prll]({{ base_path }}/docs/prll-expr/)
    2. dump
    3. cols
    4. colsDrop
    5. segmented
    6. scan

The result is updated by calling the user function as the rows stream from the
parent unit(s). The final result is passed to the next unit(s) only when the
data streaming ends in ezl::rise unit. However, if the data is segmented on key
columns i.e. all rows with the same key are next to each other, then the
reduction operation can flush out the results of the key segments to the next
unit as soon as another key appears on the input stream. This behaviour
provides pipeline parallelism which is generally not possible with typical
implementations of the reduce operation. The segmented property can be
used to enable this behaviour. Segmented reduction can also be used to
calculate and view partial aggregation results before computing the result of
the total reduction.

A reduce runs on half the processes of its parent unit when running in parallel.
The data is partitioned among the processes running reduce based on the key. 
The number of processes and the partitioning function can be changed using
[prll expressions]({{ base_path }}/docs/prll-expr/).

## reduceAll

The reduce applies the function successively for each row, however if the
reduce operation needs to have all the rows at once then reduceAll is the
option. The input parameters of the function can be vector of tuple of
key types followed by vector of tuple of value types or vector of 
key columns and value column types or const refs of these e.g.
if 1st, 2nd and 3rd columns are char, int and float, the function 
for reduceAll<3,1> can be any of the following:
 - fn(vector<float>, vector<char>, vector<int>)
 - fn(const vector<float>&, const vector<char>&, const vector<int>&)
 - fn(vector<tuple<float, char>>, vector<tuple<int>>)
 - fn(const vector<tuple<const float&, const char&>>&, const vector<tuple<const int&>>&)

  Since, vectors can be a bit expensive to copy, the use of const refs can be very
  efficient. The reduceAll has all other properties similar to reduce. ReduceAll
  buffers the rows and can be slower than reduce.

  - Properties:
    1. [prll]({{ base_path }}/docs/prll-expr/)
    2. dump
    3. cols
    4. colsDrop
    5. segmented
    6. adjacent
    7. bunch

  The **adjacent(n)** property keeps only n number of rows in the buffer for
  each key. Every time a new row arrives for a key, the oldest row is removed,
  new row is inserted and the function is called with new vector having 
  n - 1 number of prior rows and one new row. This type of operation is helpful
  for many tasks such as central difference, directions from a positions trajectory
  etc. The **bunch(n)** property calls the function with n number of new rows each
  time that many rows arrive, the last call to the function is with whatever rows
  are left and may have lesser number of rows.

  Reduction operations keep the results for each key in buffer till the end of
  data. However, if we can guarantee about ordering of data on key columns then
  the reduction operation can flush the results of a key to the next unit before
  end of data. In other words if we know that data is arranged in such a way that
  all the rows with the same keys are next to each other then the reduce only
  holds data for the current key and as soon as a new key appears it sends the
  results for the existing key to the next unit. It is very common to have
  segmented data especially in simulations and experiments.

## zip

This is similar to functional programming zip (with key) or relational data-base join.
It zips two streams together. It takes as argument the other flow object as returned
from build or run argument. The default output is all the columns of first stream
followed by all the columns of the second stream. However, column selection can be
used as with other units to select columns to output. Key can be provided for both
the strems together or separately as shown in the demoZip example.
