---
title: "Hello World"
permalink: /docs/hello-world/
excerpt: "basic examples with ezl"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

Let's look at a minimal Hello world program with ezl.

{% highlight ruby %}
ezl::rise(ezl::fromMem({"Hello world"})).dump().run();
{% endhighlight %}

Every ezl library call is inside `ezl` namespace so we will write `ezl::` for
any ezl call. We can declare `using namespace ezl` or `using ezl::rise` at the
top but this way makes it clear when we make a library call. 

Let's look at each of the constructs in the above program. We first have
`ezl::rise`. Rise is a higher order function i.e. it takes as input another
function. Rise is the original data source in a data-flow. It produces the
output on with the help of the function which takes no input parameter.
Here, inside rise we are passing `ezl::fromMem` function. This function
can take a list / container variable or a initialize list with values, it
produces a row for each item in the container. A container can be a vector,
array, map etc. We modify the property of the rise unit with `dump`. A 
dump can be added to any unit in the dataflow and it makes the output dump
to the console. dump expression takes two optional string parameters, first
for output file name and other for header message at the top of the file. `run`
is a data flow expression which makes the data-flow execute.

Let's look at another example which does some calculation as well. Following is 
an ezl program to find the summation of numbers with their square root.

{% highlight ruby %}
ezl::rise(ezl::fromMem({25.0, 100.0, 30.4}))
  .map(std::sqrt).dump("sqrt.txt", "number, sqrt")
  .map(std::plus).dump()
  .run();
{% endhighlight %}

The rise function supplies each number as a row to map. `map` is another higher
order function or unit which is same as functional paradighm map operation. It
applies the function to each row and adds the result as a new column at the
end. The first dump, writes the numbers and their square root in a file while
second output prints number, square root and their addition for each number
on console. The sqrt and plus are C++ standard library functions.

Let's try to write an ezl program to find the sixth root of a number.

```cpp
ezl::rise(ezl::fromMem({25.0, 100.0, 30.4}))
  .map(std::sqrt)
  .map(std::cbrt).dump() // error: static_assert- function and cols don't match
  .run();
```

The output rows from first map have two columns viz. input number and its
square root, while cbrt takes one number so the a static assert is raised at
compile time. The following code composes these functions together using column
selection expressions.

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
properties are equally applicable to other computing units that we will
introduce later.

If we use `colsResult` only with first map and not with second one then the
program will print the second root (input) and sixth root (output) both. But
what if we want to print number and the sixth root in each row. Following is
the program for it.

```cpp
ezl::rise(ezl::fromMem({number}))
  .map(std::sqrt)
  .map<2>(std::cbrt).colsTransform().dump() // map<2> to pass only 2nd col
  .run();
```

The second column is selected for the input to `cbrt` with map<2>. We
can select multiple columns, in any case the default output from the unit
remains to be all input columns followed by output columns. The
`colsTransform()` is a property of `map` that replaces the selected input
columns of the map (here 2nd) by the output cols returned by the function.
Without `colsTransform()` this will print number, sqrt, sixth root. Here,
`colsTransform()` can be replaced by `cols<1, 3>()` or `colsDrop<2>`.

##### Returning multiple values / columns:

A function can return multiple values / columns by returning a standard tuple.
The function that returns square root and a text message can be written as.

```cpp
auto mysqrt(int number) {
  return std::make_tuple(std::sqrt(number), "sqrt calculated");
}
```

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
  .reduce([](int x, int res) { return res + 1; }, 0) // count of total rows
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
  .reduce<3>([](int key, int x, int y, int res) { return res+ 1; }, 0)
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
    [](int key, int x, int y, int sumx, int sumy) { 
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
