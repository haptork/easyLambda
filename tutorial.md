### Getting started with ezl:

This is a introductory tutorial that assumes very little prior knowledge of
modern C++, MPI, map, reduce etc. So, skip the parts that look quite obvious. I
think it covers almost everything except particular properties of the units,
parallelism , playing around with data-flows and a few details here and there.
I am hoping to cover these and few other details, later.

Th computation in easy lambda can be seen as data flowing through different
units. The data-flow can be linear or it can have branches, merges or cycles.

The units in the data-flow are:
1. rise.
2. Map and Filter.
3. Reduce and ReduceAll.

There are various configurable properties of these units. Most of the
properties are common to all, one of such property is `.dump()` which can be
applied to any unit to print the data that is flowing out of it. Calling
`.run()` on a data flow runs it, `.build()` builds the data flow to be used
later.

Each of these units needs a callable that can be a simple C function, a lambda
or a function object.

In a data-flow rise is source/origin, which passes rows of some column types to
the next unit. Map takes every row and transforms it into new row(s). Reduce takes
a group of rows or all the rows to transform them into new row(s).

Introduction to map and filter:
===============================

Let us assume we have a data-flow with two integers as column types and we want third
column to have summation of two columns for each row. 

```cpp
int addition(int x, int y) {
 return x + y;
}

ezl::flow(twoCols) // ezl::flow adds to a prior built flow
  .map(addition).dump()
  .run();
```

In above code if the rows coming from `twoCols` are (1,2), (3, 4) then the above
code prints following:
(1, 2, 3)
(3, 4, 7)

We can instead choose to build the flow for later as follows:

```cpp
// auto is like a variable type that modern c++ compiler can deduce and you
// don't need to specify explicitly.

auto threeCols = ezl::flow(twoCols) // ezl::flow adds to a prior built flow
                   .map(addition).dump()
                   .build();
```
At this point, running the program does nothing, since pipeline is just built
not run.

Now, let us say we want to filter rows and pass only if value of third column
is greater than 5.

```cpp
ezl::flow(twoCols)
  .map(addition).dump()
  .filter([](int x, int y, int z) {
    return (z > 5);
  }).dump()
  .build();

// filter uses modern c++ lambda, it is just a function without a name, the
// return type is auto deduced. The function body is same as normal fn.
```

If we want to add a column for multiplication and other for division of
column one and two then we return two columns from a map as follows.

```cpp
auto fourCols = ezl::flow(twoCols)
                  .map([](int x, int y) {
                  return make_tuple(x * y, (double)x / y);
                }).dump()
                .build();
```

Introduction to column selection for user function
==================================================

There is some problem in the above filter, we are not using first two columns
but we have to write our function that takes them as input. What if there are
10 or even more columns and we are to operate on just single one. 

The filter in pipeline above can also be written as:

```cpp
  .filter<3>([](int z) { // selecting column three
    return (z > 5);
  })
```

This encourages reuse and this is the reason why using something like following
is quite natural. 
```cpp
  .filter<3>(ezl::gt(5));
```

If we are two say filter if column one is greater than two and column three greater
than 5, then we can write something like.
```cpp
  .filter<1, 3>(ezl::gt(2, 5));
```

Here, `ezl::gt` is a one of several library functions available for every unit 
to carry out common operations.

Column selection for user function in map is same as this.

Introduction to column selection for next unit in the data flow
===============================================================

Till, now we saw that map adds resulting cols to existing input cols, which
might not be needed at times.

Let us say we just want to return the addition of two columns and forget
about the input columns because may be just the output cols are needed in
further data-flow and in the dump.

```cpp
auto oneCol = ezl::flow(twoCols)
               .map(addition).dump().cols<3>()
               .run();
```

The `cols<...>()` property is applicable to every processing unit i.e.
filter, reduce and reduceAll.

For map we can simply do following to get just the resulting column.
```cpp
auto oneCol = ezl::flow(twoCols)
               .map(addition).dump().colsResult()
               .run();
```

Or we can do `.colsTransform()` if we want to replace the column in place.
For e.g. let us say in an input data-flow 5th column is `char` specifying
male('m') or female('f'), we want to convert this to bool false or true, we can
do the following:
```cpp
ezl::flow(someFlow)
  .map<5>([](char gender) { return (gender == 'm'); }).colsTransform()
  .build();
```

`.colsTransform()` can be done with multiple columns as well in which case
multiple columns that are selected get transformed into resulting col(s). If
all the columns are selected for function then `colsResult` and `colsTransform`
give same results.

Introduction to reduce
======================

Let us say we want to add all the rows in the `oneCol` data-flow.
```cpp
ezl::flow(oneCol)
  .reduce(addition, 0)
  .build();
```
Here, 0 is the initial value. The reduce function receives each row and the
prior value of result. It returns the updated result which is passed next 
time a new row is streamed in. At the end of data the resulting row with
final addition is passed to next unit(s).

A count of rows can be done by simply incrementing the result each time.
```cpp
ezl::flow(oneCol)
  .reduce([](int x, int res) { return res + 1; }, 0) // count of total rows
  .build();
```
or
```cpp
ezl::flow(oneCol)
  .reduce(ezl::count(), 0)
  .build();
```

The reduce operation can be applied independently to different groups. These
groups are decided by the value of key columns.

Let us say in threecols data-flow we want to check how many pair of integers
result in a particular value of addition (third col).
For e.g. if rows in threeCols are (3, 4, 7), (2, 5, 7), (5, 3, 8)
we want output to be (7, 2), (8, 1)
we want to calculate count of rows in each group with key column 3.
```cpp
ezl::flow(threeCols)
  .reduce<3>([](int key, int x, int y, int res) { return res+ 1; }, 0)
  .build();
```
or
```cpp
ezl::flow(threeCols)
  .reduce<3>(ezl::count(), 0)
  .build();
```

Here, we take column three as key, by default all other cols are value cols.
The function parameter are key cols, value cols, result cols. The returned
rows are of type key cols, resulting cols.

One can select multiple key cols. Result also can have multiple cols.

Let us say we want to add cols one and two for all the rows that have same
value for column three.
```cpp
ezl::flow(threeCols)
  .reduce<3>(
    [](int key, int x, int y, int sumx, int sumy) { 
      return make_tuple(x + sumx, y + sumy); 
    }
    , 0, 0)
  .build();
```
or
```cpp
ezl::flow(threeCols)
  .reduce<3>(ezl::sum(), 0, 0)
  .build();
```

Two zeros are for initial value of two resulting columns, respectively.

Reduction of rows for which the successive application of a function is not applicable
can be done using ReduceAll.

```cpp
ezl::flow(threeCols)
  .reduceAll<3>([](int key, vector<int> x, vector<int> y) { return int(y.size()); })
  .build();
```
or
```cpp
ezl::flow(threeCols)
  .reduceAll<3>([](int key, vector<tuple<int, int>> y) { return int(y.size()); })
  .build();
```

So essentially, the parameters are key, vector of value cols. The vectors can be
separate vector of column types or vector of tuple of column types according to the
convenience of the operation to be carried out.

Reduces have same kind of column selection for resulting columns as given with
examples of map and filter.

Finally, key and value columns both can be selected with a syntax like following:
`.reduce<ezl::key<1, 2>, ezl::val<3, 5>>(...)`.

Introduction to rise
====================

We have seen a lot of data-flows which manipulate the input rows that flow into
them but there must be some origin of the data. Rise is the origin of a flow.

It essentially takes a function that returns {row, isEndOfData} or a vector of rows
till EndOfData implied by returning an empty vector.

There are hardly any instances where a custom rise function will be needed. There
are plenty of rise functions available with ezl.

ReadFile: 
---------

So let us say we have a text file containing one string, two ints and a float
value separated by commas and tabs. We can load the file as follows:
```cpp
ezl::rise(
  ezl::readFile<string, array<int, 2>, float>("files*.txt").colSeparator(",\t"))
  .build();
```

readFile is quite comprehensive it takes care of errors in reading, it has
option to have strictSchema (reject rows that have different size of column)
or noStrictSchema (fill in defaults if less cols, or ignore if more cols),
parallel read. It can even sort of denormalise or attach a header information
to every row which is important for simulation dumps where timesteps are
generally written at the top and system value follows it. Check 
[demoReadFile](examples/demoReadFile.cpp) for more on these options.

loadMem:
-------
This loads rows from a container or intializer list.
```cpp
ezl::rise(
  ezl::loadMem({1,2,3})
  .build();
```

The rise in above data-flow will stream three rows having a single column of
type integer to the data-flow. 
Similarly, a vector or array or any other container can be passed to it.
A container of type tuple, streams rows with multiple cols to the data-flow.
Check [demoIO](examples/demoIO.cpp) to view more examples on this.

kick:
----
`kick(N)` streams N number of empty rows to the next column. 
It takes care of the parallelism so that one can decide if total of N times
over all the processes or N times on each process. By default the N is shared
among the processes.
In addition to [demoIO](examples/demoIO.cpp), [pi](examples/pi.cpp) example also uses this.

A few things more:
=================

- Instead of `run()`, if one does `runResult()` the rows flowing out of the final
  unit get returned.
- If you see a long compile time error, just scroll to the top, most likely
  you will see a static_error saying either col selection is not possible or
  function params and cols do not match.
- To return zero, one or multiple rows, return a vector of rows from any unit.
  This is also applicable for `reduce()`.
- For big column types it is good to have params as const bigtype&.
- If you are returning a big object from reduce and are worried about creating
  a new one every time you update the result for a new row. Then worry not. You
  can have your resulting type as bigtype&, update it in-place and return the
  reference. If using auto as return type or lambda don't forget to explicitly 
  specify return type to be auto&.
