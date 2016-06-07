---
title: "Hello World"
permalink: /docs/hello-world/
excerpt: "basic examples with ezl"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

We discuss basic set of rules that ezl is based on with the help of
simple examples.

Let's look at a minimal Hello world data-flow with ezl.

{% highlight ruby %}
using ezl::rise;
using ezl::fromMem;
rise(fromMem({"Hello world"})).dump()
  .run();
{% endhighlight %}

Every ezl library call is inside ezl namespace. In the above example we have
namespace qualifier for each call at the beginning such as `using ezl::rise`.
The other ways can be - qualifying each call separately such as
`ezl::rise(...)` or declaring `using namespace ezl;` at the top. We will use
either of the first two to make it clear which calls are ezl calls.

Let's look at each of the step in the above program. We first have
a rise which is a higher order function i.e. it takes another function as input. 
Rise is the original data source in a data-flow. It produces the
output with the help of a function which takes no input parameter.
Here, we are passing `fromMem` library function to it. This function
can take a list / container variable or an initialize list with values, it
produces a row for each item in the container. A container can be a vector,
array, map etc. We have added dump property to the rise unit. A
dump can be added to any unit in the dataflow and it displays the output of the
unit on the console. The dump expression takes two optional string parameters,
first for output file name and other for header message at the top of the file.
At the end we call run which executes the data data-flow.

Let's look at another example which does some calculation as well. Following is 
an ezl program to find the summation of numbers with their square root.

{% highlight ruby %}
ezl::rise(ezl::fromMem({25.0, 100.0, 30.4}))
  .map(std::sqrt).dump("sqrt.txt", "number, sqrt")
  .map(std::plus).dump()
  .run();
{% endhighlight %}

The rise function streams each number as a row to map. map is another higher
order function or unit which is similar to functional paradighm map operation. It
applies the function to each row and adds the result as a new column at the
end of the row. The first map with dump, writes the numbers and their square
root in a file. The second map prints (number, square root and their addition)
for each row on console. The sqrt and plus are C++ standard library functions.

### Sixth Root

Let's write an ezl data-flow to find the sixth root of a number.

{% highlight ruby %}
ezl::rise(ezl::fromMem({25.0, 100.0, 30.4}))
  .map(std::sqrt)
  .map(std::cbrt).dump("doesn't compile")
  .run();
{% endhighlight %}

The output rows from first map have two columns viz. input number and its
square root, while function cbrt takes one number. This data-flow is not well
formed so a static assert is raised at compile time. easyLambda gives helpful
static_error mesaages for ill-formed data-flows that can be viewed at the
beginning of the compile error text.

The following data-flow composes these functions together using column selection
expressions for the output.

{% highlight ruby %}
ezl::rise(ezl::fromMem({number}))
  .map(std::sqrt).colsResult()
  .map(std::cbrt).colsResult().dump()
  .run();
{% endhighlight %}

The colsResult is a property of map unit. It makes only the result pass
to the next unit rather than adding result to the input columns. The program
prints the sixth root. We can select output columns by their indices as well.
In place of `colsResult()` we could have written `cols<2>()` or
`colsDrop<1>()`. The cols and colsDrop properties are also applicable to other
computing units like filter and reduce.

If we use `colsResult` only with first map and not with second one then the
program will print the square root (input) and sixth root (output) both. But
what if we want to print number and the sixth root in each row. Following is
the program for it.

{% highlight ruby %}
ezl::rise(ezl::fromMem({number}))
  .map(std::sqrt)
  .map<2>(std::cbrt).colsTransform().dump()
  .run();
{% endhighlight %}

The second column is selected for the input to cbrt function with map<2>. We
can select multiple columns. The default output from the unit
is all input columns followed by output columns. The colsTransform is a
property of map that replaces the selected input columns of the map (here
2nd) by the output cols returned by the function.  Without colsTransform
this will print number, sqrt, sixth root. Here, `colsTransform()` can be
replaced by `cols<1, 3>()` or `colsDrop<2>`.

The example has introduced machinery for column selection for composition in
ezl which is quite useful for reuse. It is the column selection that makes
reuse of generic algorithms in easyLambda for any data possible with minimal
syntax.

### Sqrt and Cbrt in different Columns

Folowing is a data-flow to add a column for square root and another for cube
root using a single map.

{% highlight ruby %}
ezl::rise(ezl::fromMem({25.0, 100.0, 30.4}))
  .map([](double num) {
    return std::make_tuple(sqrt(num), cbrt(num));
  }).dump()
  .run();
{% endhighlight %}

A function can return multiple values / columns by returning a standard tuple.
For square root and cube root, it is better to use two maps one for sqrt and
another for cbrt with column selection since the two operations are not
related. However, some results are truly composed of more than one value.
Tuples are the standard way of returning multiple values in ezl as well as in
general.

### Sqrt and Cbrt in Different Branches

Let's say we want to calculate square root and cube root with different maps
that both have same source unit. We want to have them as branches at same
level, since they are independent operations and don't actually have to be
executed one after the other.

{% highlight ruby %}
auto src = ezl::rise(ezl::fromMem({25.0, 100.0, 30.4}))
            .build();

ezl::flow(src)
  .map(std::sqrt).dump("sqrt.txt").build();

ezl::flow(src)
  .map(std::cbrt).dump("cbrt.txt").run();
{% endhighlight %}

We build a source unit with `build()`. Building does not execute the data-flow,
instead it returns the last unit of the data-flow. With `flow(src)` we resume
the src data-flow and add to it a map with sqrt and then another map with cbrt.
When we run the data-flow with cbrt, the data from the rise starts streaming to
both the child branches of it and results get saved into files they are dumping
to. With parallelism property that we will see later, we can run two branches in
different processes simultaneously. Building a data-flow can be useful in many
cases like for cyclic flows, for building a data-flow and then running it for
different data iteratively etc. We will see some examples of these in practical
problems as well.

### Data-flow to return sixthRoot

Following is a function that returns sixth root of a number passed to it.

{% highlight ruby %}
auto sixthRoot(double number) {
  auto res = 0.0;
  std::tie(res) = ezl::rise(ezl::fromMem({number}))
                    .map(std::sqrt).colsResult()
                    .map(std::cbrt).colsResult()
                    .runResult()[0];
  return res;
}
{% endhighlight %}

To get the end result of the data-flow as a return value one can use
`.runResult()` in place of `.run()`. The return value is always a vector of
tuple of various values returned. With [0] at the end we get the first tuple
of the result and with tie we assign the tuple values to the variables inside
the tie.

### Count

Let us say from a data source with rows having two columns, we want to count
the number of rows that addup to more than 5. Here, is the data-flow for this.

{% highlight ruby %}
using ezl::rise;
using ezl::fromMem;
using std::make_tuple;
rise(fromMem({make_tuple(4,3), make_tuple(2,5), make_tuple(2,1)}))
  .map(std::plus)
  .filter<3>(ezl::gt(5))
  .reduce(ezl::count(), 0).dump()
  .build();
{% endhighlight %}

Let us dissect this data-flow. We throw a bunch of tuples to the fromMem
function in an initializer list. Each tuple provides a row with two columns.
The map unit then adds the two columns and passes the result along with input
numbers. 

Filter is a higher order function that has same meaning as in functional
programming. Similar to map it takes every row and calls the predicate (a
function that returns bool value) with it. The rows for which return value is
true get streamed to the next unit. In the above data-flow, inside filter third
column is selected as a parameter to predicate, this is similar to input column
selection for map. The predicate gt is an ezl generic function object for
greater than. We pass 5 as the reference number. If we were filtering on let's
say first column be greater than 2 and third column be greater than 5 then our
filter would be `.filter<1,3>(ezl::gt(2, 5))`.  Similar, function objects for
less that (lt) and equals (eq) are also available with ezl algorithms for
filter. 

Next, we have a reduce unit to count the filtered rows. reduce is another
higher order function or unit which is similar to functional paradighm fold
function. It finds a single aggregated result after calling the user function
on all the rows. The reduce takes as parameter a function and the initial value
of the result. For each row it calls the user function with key input columns,
value input columns and prior result.  The key is used to group the rows and
then a single result is obtained for each group.  We will see the grouping in
next example. Here, we are counting all the rows so we don't require grouping.
We pass a generic function object count and zero as initial value for the
result. For each row, count returns an incremented value of prior result. The
end result is the total rows which gets printed on the console.

Let's say we want to count the rows for each value of addition. Since, (4, 3)
and (2, 5) both add to 7 while (2, 1) adds to 3, the desired result is
(7, 2) and (3, 1). Follwing is the data-flow for this.

{% highlight ruby %}
using ezl::rise;
using ezl::fromMem;
using std::make_tuple;
rise(fromMem({make_tuple(4,3), make_tuple(2,5), make_tuple(2,1)}))
  .map(std::plus)
  .reduce<3>(ezl::count(), 0)
  .build();
{% endhighlight %}

If we were to write count function specific to our case ourselves then it
can be written as:

{% highlight ruby %}
  .reduce<3>([](int key, int val1, int val2, int res) { 
    return res + 1; 
  }, 0)
{% endhighlight %}

Here, we take column three as key. By default all other cols are value cols.
The function parameter are key cols, value cols, result cols. The returned
rows are of type key cols, resulting cols.

We can select value colums as well and the data-flow can be rephrased as follows.

{% highlight ruby %}
  .reduce<key<3>, val<>>([](int key, int res) { 
    return res + 1; 
  }, 0)
{% endhighlight %}

One can also select multiple key or value columns. The following data-flow finds
summation of all the rows for each column.

{% highlight ruby %}
using ezl::rise;
using ezl::fromMem;
using std::make_tuple;
rise(fromMem({make_tuple(4,3), make_tuple(2,5), make_tuple(2,1)}))
  .reduce(ezl::sum(), 0, 0)
  .run();
{% endhighlight %}

Two zeros are for initial value of two resulting columns, respectively.

There are reductions that might take multiple steps if we use the reduce
unit that recieves one row at a time. However, if the data is small then there
is no reason to do that. For these cases we have reduceAll higher order
function. To return the count of rows with same value of third column, the
data-flow will look like below.

{% highlight ruby %}
using ezl::rise;
using ezl::fromMem;
using std::make_tuple;
rise(fromMem({make_tuple(4,3), make_tuple(2,5), make_tuple(2,1)}))
  .map(std::plus)
  .reduceAll<3>([](int key, vector<int> a, vector<int> v) { 
    return int(a.size()); 
  });
{% endhighlight %}

or

{% highlight ruby %}
using ezl::rise;
using ezl::fromMem;
using std::make_tuple;
rise(fromMem({make_tuple(4,3), make_tuple(2,5), make_tuple(2,1)}))
  .map(std::plus)
  .reduceAll<3>([](int key, vector<tuple<int, int>> val) { 
    return int(val.size()); 
  });
{% endhighlight %}

So essentially, the parameters of user function in reduceAll are key, vector of
value cols. The vectors can be separate vector of column types or vector of
tuple of column types according to the convenience of the operation to be
carried out. Key and value columns can be selected similar to reduce unit.
ReduceAll library function objects include summary, histogram and correlation.
These are pretty useful if you want to get the idea of the data. As for most of
the library function these are applicable to any number of columns. There is
a very useful property of reduceAll `adjacent` that we will introduce in later
examples.

The property `cols<...>()` can be applied to any reduce unit for columns that
need to be passed ahead.

### Multiple rows

For a source with two columns, one string and another a number, the following
data-flow duplicates the row as many times as the corresponding number in
the row.

{% highlight ruby %}
using ezl::rise;
using ezl::fromMem;
using std::make_tuple;
rise(fromMem({make_tuple("yo", 0), make_tuple("ezl", 2), make_tuple("one", 1)}))
  .map([](string str, int times) {
    return vector<string>(times, str);
  }).cols<3,2>().dump()
  .run();
{% endhighlight %}

The rise streams the rows with string and integer columns to the map.
The map user function returns a vector of strings which is treated as returning
multiple rows. The output column selection works the same way as it does for
a single return value. Each output row has third and second column i.e. output
string and input number. The output of the program will be (("ezl", 2), ("ezl",
2), ("one", 1)). The first row ("yo", 0) doesn't appear at all.

A vector of tuple can be returned for returning multiple rows with multiple columns.
For returning vector as a column, a tuple of vector is to be returned. This applies
to any higher order function like reduce, reduceAll in ezl.
