2 Map and Reduce
==================

The section builds upon the basics of data-flow with ezl as introduced in [prior section on data-flow](1_dataflow.md).

The computation in easy lambda can be seen as data flowing through different
units. These units are:

1. Rise.
2. Map and Filter.
3. Reduce and ReduceAll.

There are various configurable properties of these units. Most of the
properties are common to all for e.g. dump(), run(), build() etc.

Each of these units need a callable that can be a simple C function, a lambda
object or a function object.

In a data-flow rise is source/origin, which passes rows of some column types to
the next unit. Map takes every row and transforms it into new row(s). Reduce takes
a group of rows or all the rows to transform them into new row(s).

#### A real world example

Let us take an example from [Cods2016](http://ikdd.acm.org/Site/CoDS2016/datachallenge.html). A stripped version of original
data-file is given with ezl [here](../../data/datachallenge_cods2016/train.csv). The data contains student profiles
with scores, gender, job-salary, city etc.

```cpp
auto scores = ezl::fromFile<char, array<float, 3>>(fileName)
                .cols({"Gender", "English", "Logical", "Domain"})
                .colSeparator("\t");

ezl::rise(scores)
  .filter<2>(ezl::gtAr<3>(0.F))              // filter valid domain scores
    .reduceAll<1>(ezl::summary()).dump("", 
      "Gender split of scores(E, L, D) resp.\n(gender|count|avg|std|min|max)")
    .oneUp()
  .map<1>([] (char gender) { 
    return float(gender == 'M' || gender == 'm');
  }).colsTransform()
  .reduceAll(ezl::corr<1>()).dump("corr.txt",
                               "Corr. of gender with scores\n(gender|E|L|D)")
  .run();
```

The code prints the summary of each score for male and female separately. It
writes a file "corr.txt" with the correlation of English, logical and domain
scores with respect to gender. 

We can find similarity of the above code with steps in spreadsheet analysis or
with an SQL query. In the above code we load certain columns from the file to
work with. We select the columns on which we want to apply filter criteria. We
select columns that we want to transform and use `colsTransform()` to do it
in-place. We reduce to find aggregate values of multiple rows like correlation
and summary. The summary for male and female are found separately by selecting
column 1st as grouping key in the reduce. A different analysis of the same data
is given in [cods2016.cpp example](../../examples/cods2016.cpp).

By the end of this section, we will be able to understand the details of the
above example.

#### Introduction to map and filter

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

In above code if the rows coming from `twoCols` are (1,2), (3, 4) then it
prints following:
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

At this point, running the program does nothing, since data-flow is just built
not run. `ezl::flow(threeCols).run()` will run the data-flow later.

If we want to add a column for multiplication and another for division of
column one and two then we return two columns from a map as follows.

```cpp
auto fourCols = ezl::flow(twoCols)
                  .map([](int x, int y) {
                  return make_tuple(x * y, (double)x / y);
                }).dump()
                .build();
```

Now, let us say we want to filter rows and pass only if value of the third column
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

There is some problem in the above filter, we are not using first two columns
but we have to write our function that takes them as input. What if there are
10 or even more columns and we are to operate on just single one. 

A better written filter would be:

```cpp
  .filter<3>([](int z) { // selecting column three
    return (z > 5);
  })
```

The column selection facilitates composition and encourages reuse. Hence, the
generic functions provided by ezl for maps, filters and reduces can be
extensively used. 

Following is the filter using library function for greater than.

```cpp
  .filter<3>(ezl::gt(5));
```

If we want to filter for rows that have column one greater than 2 and column three greater
than 5, then we can write something like.
```cpp
  .filter<1, 3>(ezl::gt(2, 5));
```

#### Introduction to reduce

Let us say we want to add all the rows in the `oneCol` data-flow.

```cpp
ezl::flow(oneCol)
  .reduce(addition, 0)
  .build();
```

Here, 0 is the initial value. The reduce function receives each row and the
prior value of result as its parameters. It returns the updated result which is
passed next time a new row is streamed in. At the end of data the resulting row
with final addition is passed ahead in the data-flow.

A count of rows can be done by simply incrementing the result each time.

```cpp
ezl::flow(oneCol)
  .reduce([](int res, int x) { return res + 1; }, 0) // count of total rows
  .build();
```

A better way is to use library function count.

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
  .reduce<3>([](int res, int key, int x, int y) { return res+ 1; }, 0)
  .build();
```

Again, it is better to use library function:

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
    [](int sumx, int sumy, int key, int x, int y) { 
      return make_tuple(x + sumx, y + sumy); 
    }
    , 0, 0)
  .build();
```

Again,

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

ReduceAll library functions include summary, histogram and correlation. These
are pretty neat if you want to get the idea of the data. As for most of the
library function these are applicable to any number of columns.

The property `cols<...>()` can be applied to any reduce unit for columns that
need to be passed ahead.

Finally, key and value columns both can be selected with a syntax like following:
`.reduce<ezl::key<1, 2>, ezl::val<3, 5>>(...)`.

#### Introduction to rise

Rise takes a function with no parameter that returns {row, isEndOfData} or a
vector of rows in which case EndOfData is implied by returning an empty vector.
The function in Rise is called till EndOfData.

Rise functions for many common scenarios are already given  with ezl such as
for loading tabulated data from files, for loading list of data from memory,
for calling the next unit n number of times without any parameter. All
of these functions work in parallel execution.

##### FromFile: 

Let us say we have a text file containing one string, two ints and a float
value separated by either commas or tabs or both. We can load the file as follows:

```cpp
ezl::rise(
  ezl::fromFile<string, array<int, 2>, float>("files*.txt").colSeparator(",\t"))
  .build();
```

fromFile is quite comprehensive it takes care of errors in reading, it has
option to have strictSchema (reject rows that have different size of column)
or set it to false (fill in defaults if less cols, or ignore if more cols),
parallel read. It can even sort of denormalise or attach a header information
to every row which is important for simulation dumps where timesteps are
generally written at the top and rows follows it. Check
[demoFromFile](../../examples/demoFromFile.cpp) for more on these options.

##### fromMem:
This loads rows from a container or intializer list.

```cpp
ezl::rise(ezl::fromMem({1,2,3}))
  .build();
```

The rise in above data-flow will stream three rows having a single column of
type integer to the data-flow. 
Similarly, a vector or array or any other container can be passed to it.
A container of type tuple, streams rows with multiple cols to the data-flow.
Check [demoIO](../../examples/demoIO.cpp) to view more examples on this.

##### kick:
`kick(N)` streams N number of empty rows to the next column. 
It takes care of the parallelism so that one can decide if total of N times
over all the processes or N times on each process. By default the N is shared
among the processes.

```cpp
ezl::rise(ezl::kick(20))
  .map([] { return make_tuple("twenty", "20"); })
  .build();
```

In addition to [demoIO](../../examples/demoIO.cpp), [pi](../../examples/pi.cpp) example also uses this.

#### A few things more:

- If you see a long compile time error, just scroll to the top, most likely
  you will see a `static_error` saying either col selection is not possible or
  function params and cols do not match.
- To return zero, one or multiple rows, return a vector of rows from any unit.
  This is also applicable for `reduce()`.
- For big column types it is good to have params as `const bigtype&`.
- If you are returning a big object from reduce and are worried about creating
  a new one every time you update the result for a new row. Then worry not. You
  can have your resulting type as `bigtype&`, update it in-place and return the
  reference. While returning the reference if you are using auto as return type
  or lambda function, don't forget to explicitly specify return type to be `auto&`.
