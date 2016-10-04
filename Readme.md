# ezl: easyLambda
> Parallel data processing made easy using functional and dataflow programming with modern C++

Welcome to easyLambda and thanks for your interest. The site aims to be a
comprehensive guide for easyLambda.

## What is easyLambda

EasyLambda is *header only C++14* library for data processing in parallel with
*functional list operations* (map, filter, reduce, scan, zip) that are tied
together in *type--safe dataflow*. 

EasyLambda is parallel, it scales from multiple cores to *hundreds of distributed*
nodes *without any need to deal with parallelism* in user code.

EasyLambda is fast. It has *minimal overhead in serial execution* and builds upon
high performance MPI parallelism that is known to be *more efficient than any
other comparable work*
[[1]](http://www.sciencedirect.com/science/article/pii/S1877050915017895).

EasyLambda is *expressive and succinct*, thanks to the *column selection* for
composition of functions and many *generic algorithms* such as *configurable
parallel file reader*, predicates, correlation, summary etc.

EasyLambda is intuitive and easy to understand with its *uniform property based*
(or [ExpressionBuilder](http://martinfowler.com/bliki/ExpressionBuilder.html))
interface for everything from configuring parallelism to changing behavior of
generic algorithms to routing dataflow.

EasyLambda is easily *interoperable* with other libraries like standard library or
raw MPI code, since it uses *standard data types* and enforces no special
structure, data-types or requirements on the user functions.

## Why easyLambda

EasyLambda is a good fit for the following tasks:
+ table/list processing and analysis from CSV or flat text files.
+ post-processing of scientific simulation results.
+ running iterative machine learning algorithms.
+ parallel type-safe data reading.
+ to play with dataflow programming and functional list operations.

Since, it can smoothly interoperate with other libraries, it is possible to
add distributed parallelism using easyLambda to the existing libraries or
codebase when its programming abstraction fits well e.g. it can be used along
with bare MPI code or with a machine learning library to add distributed training
and testing.

EasyLambda will also interest you if you 
+ are a modern C++ enthusiast
+ want to dabble with metaprogramming
+ like functional and dataflow programming
+ have cluster resources that you want to put to use in everyday tasks without much effort.
+ have always wanted a high-level MPI interface.

#### Benchmarks

Check out the benchmarks 
[![benchmarks](doc/benchmarks.png)](https://haptork.github.io/easyLambda/docs/benchmarks/)


## Getting Started

Check out the [Getting Started](https://haptork.github.io/easyLambda/docs/quick-start-guide/)
section of the library webpage to know how to install and begin with easyLambda. The library
can also be used on aws elastic cloud or single instance. 


# Examples

A detailed walkthrough of the library is given [here](https://haptork.github.io/easyLambda/docs/hello-world/),
The [examples directory](examples) contains various examples and demonstrations
for features and options along with explanations.

Here we mention some examples in short.


## [Example wordcount](examples/wordcount.cpp)

The following program calculates frequency of each word in the data files.

```cpp
  ezl::rise(fromFile<string>(argv[1]).rowSeparator('s').colSeparator(""))
    .reduce<1>(ezl::count(), 0).dump()
    .run();
```

The dataflow pipeline starts with `rise` and subsequent operations are added to it.
In the above example, the pipeline begins by reading in data from the specified 
file(s). `fromFile` is a library function that takes column types and the specified 
file(s) glob pattern as input and reads the file(s) in parallel. It has a lot of
properties for controlling data-format, parallelism, denormalization etc
(shown in [demoFromFile](examples/demoFromFile.cpp)).

In `reduce` we pass the index of the key column to group by, the library function
for counting and initial value of the result.



## [Example pi (Monte-Carlo)](examples/pi.cpp)

Following is a dataflow for calculating pi using Monte-Carlo method.

```cpp
ezl::rise(ezl::kick(10000)) // 10000 trials shared over all processes
  .map([] { 
    return pow(rnd(), 2) + pow(rnd(), 2);
  })
  .filter(ezl::lt(1.))
  .reduce(ezl::count(), 0)
  .map([](int inCircleCount) { 
    return (4.0 * inCircleCount / 10000); 
  }).dump()
  .run();
```

The dataflow starts with rise in which we pass a library function to call the
next unit a number of times. The steps in the algorithm have been expressed
with the composition of small operations, some are common library functions
like `count()`, `lt()` (less-than) and some are user-defined functions specific
to the problem.



## [Example CSV stats](examples/cods2016.cpp)

Here is another example from
[cods2016](http://ikdd.acm.org/Site/CoDS2016/datachallenge.html). A stripped
version of the input data-file is given with ezl
[here](data/datachallenge_cods2016/train.csv). The data contains student
profiles with scores, gender, job-salary, city etc.

```cpp
auto scores = ezl::fromFile<char, array<float, 3>>(fileName)
                .cols({"Gender", "English", "Logical", "Domain"})
                .colSeparator("\t");

ezl::rise(scores)
  .filter<2>(ezl::gtAr<3>(0.F))   // filter valid domain scores > 0
  .map<1>([] (char gender) {      // transforming with 0/1 for isMale
    return float(gender == 'm');
  }).colsTransform()
  .reduceAll(ezl::corr<1>())
    .dump("", "Corr. of gender with scores\n(gender|E|L|D)")
  .run();
```

The above example prints the correlation of English, logical and domain scores
with respect to gender. We can find similarity of the above code with steps in
a spreadsheet analysis or with SQL query. We select the columns to work with
viz. gender and three scores. We filter the rows based on a column and predicate.
Next, we transform a selected column in-place and then find an aggregate property
(correlation) for all the rows.

----

## Contributing

Suggestions and feedback are welcome. Feel free to contact via mail or issues
for any query.

The project improvements are around the following directions:

+ compile time optimization
+ use of specialized data structures in various units like reduce etc.
+ addition of more examples
+ design simplifications
+ parallelism optimization
+ code reviews
+ documentation

Future extenstions to incorporate in the project are as follows:

+ fault tolerance
+ algorithms / functions to plot streaming and buffered data
+ MPI single-sided communications
+ Experiments to extend current programming abstraction to cover more problems like domain-decomposition etc. 

Check [internals](https://haptork.github.io/easyLambda/docs/internals) and
[blog](https://haptork.github.io/easyLambda/posts/) for design and
implementation details.


## Acknowledgments

A big thanks to cppcon, meetingc++ and other conferences and all C++ expert
speakers, committee members and compiler implementers for modernising C++ and
teaching it with so much enthusiasm. I had fun implementing this, hoping you
will have fun using it. Looking forward to learn more from the community.

I wish to thank [eicossa](https://github.com/eicossa) and Nitesh for their
(less online, more offline :P) contributions.
