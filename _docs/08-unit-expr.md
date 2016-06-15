---
title: "Unit Expressions"
permalink: /docs/unit-expr/
excerpt: "Refernce on unit expressions"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

A new computing unit can be added anywhere in the data-flow with the unit
expressions. They finalize the prior data-flow unit. These are higher order
functions ie. take another function as input. The units are based on functional
list operations. 

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
are no copies made even for shuffle as the rows are represented internally by
tuple of const references. The parameters for reduce function are similar but
not exactly same and are described in the specific sections below.

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
    2. [dump]({{ base_path }}/docs/dump-expr/)
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
    2. [dump]({{ base_path }}/docs/dump-expr/)
    3. cols
    4. colsDrop

The filter has input selection similar to map which allows for reuse of generic
function objects very often.  Similar to map it streams the rows as they come
and runs inprocess by default that can be changed with prll property.

## reduce

A reduce is used for operations that take multiple rows as input such as
common aggregation operations like count, sum, correlation etc. It takes
function and initial value for the result columns as the parameters. The value
of results is initially set using parameters given to reduce and are updated
for each row with the returned value of the function. The final value of the
result is passed to the next unit.

The reduce result can be calculated for each sub-list or group separately such
as finding the count of atoms rather for each time-step separately than the
total atoms all over. This is termed sometimes as reduce by key or segmented
reduce. This is similar to groupby queries of SQL. The key columns to group
the rows can be selected by indices e.g. reduce<3, 1> selects third and first
column as the key. All the non-key columns are value columns by default. However
with key and value both can be selected e.g. reduce<ezl::key<3,1>, ezl::val<4>>.
The input parameters of the function are key and value columns followed by
result columns, that can be types, const refs or the tuple of
key, tuple of value followed by tuple of result or tuple of const refs e.g.
if 1st, 2nd and 3rd columns are char, int and float and result is of type
bool, the function for reduce<3,1> can be any of the following:

- fn(float, char, int, bool),
- fn(const float&, const char&, const int&, const bool&), 
- fn(tuple<float, char>, tuple<int>, tuple<bool>)
- fn(const tuple<const float&, const char&>&, const tuple<const int&>&, 
     const tuple<const bool&>&)

Since, the result is updated for every row and the cost of constructing a new
result and returning it can be higher for some return types, the result
parameter can be a reference type without const and can be returned as
reference after modifying. The key or value has to be const reference or it is
a compile time error, since a single row may stream to multiple units, so ezl
doesn't allow mutating the input rows. The reduce operation minimizes the copy
operations to least.

  - Properties:
    1. [prll]({{ base_path }}/docs/prll-expr/)
    2. [dump]({{ base_path }}/docs/dump-expr/)
    3. cols
    4. colsDrop
    5. ordered

The result is updated by calling the user function as the rows stream from the
parent unit(s). The final result is passed to the next unit(s) only when the
data streaming ends in ezl::rise unit. However, if the input rows are already
ordered by the key i.e. all the keys appear contigously then it makes no sense
to buffer the result till end of data. The **ordered** property solves just this
issue and makes the current result as soon as the key changes.

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
    2. [dump]({{ base_path }}/docs/dump-expr/)
    3. cols
    4. colsDrop
    5. ordered
    5. adjacent
    5. bunch

  The **adjacent(n)** property keeps only n number of rows in the buffer for
  each key. Every time a new row arrives for a key, the oldest row is removed,
  new row is inserted and the function is called with new vector having 
  n - 1 number of prior rows and one new row. This type of operation is helpful
  for many tasks such as central difference, directions from a positions trajectory
  etc. The **bunch(n)** property calls the function with n number of new rows each
  time that many rows arrive, the last call to the function is with whatever rows
  are left and may have lesser number of rows.
