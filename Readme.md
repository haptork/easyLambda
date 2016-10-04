# ezl: easyLambda
> Parallel data processing made easy using dataflows in modern C++

## Design Goals
EasyLamdba is designed to be usable and performant. 

### Usability 
The usability goals are :

+ composable and functional interface
+ decoupled I/O and parallelism from algorithm logic
+ accessible to a C programmer

### Performance
The peformance goals are : 

+ minimal runtime overheads
+ efficient parallel scalability

EasyLambda combines the efficiency of MPI with the usability of a high-level
programming abstraction. You can write readable parallel code with good runtime
performance. The header-only library is based on [dataflow programming](https://en.wikipedia.org/wiki/Dataflow_programming) 
with functional list operations while making best use of modern C++ advances to have a minimal
overhead implementation with succinct interface. It seamlessly scales from
multiple cores to hundreds of distributed nodes using efficient MPI
parallelism.

## Targeted users

EasyLambda can be used for the following tasks :
+ table/list-processing
+ post-processing for physics simulation results
+ running iterative machine learning algorithms
+ parallel type-safe data reading
+ try out dataflow programming

EasyLambda will also interest you if you 
+ are a modern C++ enthusiast
+ like functional programming
+ have always wanted a high-level MPI interface


## Performance

Check out the benchmarks 
[![benchmarks](doc/benchmarks.png)](https://haptork.github.io/easyLambda/docs/benchmarks/)


## Getting Started

Check out the 
[Getting Started](https://haptork.github.io/easyLambda/docs/quick-start-guide/)
section of the library webpage to know how to install and use it. The library
can also be used on aws elastic cloud or single instance. 



# Examples

A detailed walkthrough of the library is given [here](https://haptork.github.io/easyLambda/docs/hello-world/),
The [examples directory](examples) contains various examples and demonstrations
for features and options along with explanations.

Here we mention some examples in short.
The following program calculates frequency of each word in the data files.

## [Example wordcount](examples/wordcount.cpp)
```cpp
  ezl::rise(fromFile<string>(argv[1]).rowSeparator('s').colSeparator(""))
    .reduce<1>(ezl::count(), 0).dump()
    .run();
```

The data-flow pipeline starts with `rise` and subsequent operations are added to it.
In the above example, the pipeline begins by reading in data from the specified 
file(s). `fromFile` is a library function that takes column types and the specified 
file(s) glob pattern as input and reads the file(s) in parallel. It has a lot of
properties for controlling data-format, parallelism, denormalization etc
(shown in [demoFromFile](examples/demoFromFile.cpp)).

In `reduce` we pass the index of the key column to group by, the library function
for counting and initial value of the result.

Following is the data-flow for calculating pi using Monte-Carlo method.

## [Example pi (Monte-Carlo)](examples/pi.cpp)
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

Here is another example from
[cods2016](http://ikdd.acm.org/Site/CoDS2016/datachallenge.html). A stripped
version of the input data-file is given with ezl
[here](data/datachallenge_cods2016/train.csv). The data contains student
profiles with scores, gender, job-salary, city etc.

## [Example cods2016](examples/cods2016.cpp)
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

# Contributing

Suggestions and feedback are welcome. Feel free to contact via mail or issues
for any query.

The project needs improvement in the following areas : 

+ data handling
+ compile times
+ documentation

The project needs extensions to incorporate the following features : 

+ fault tolerance
+ MPI single-sided communications 


Check [internals](https://haptork.github.io/easyLambda/docs/internals)
for design and implementation details.


# Acknowledgments

A big thanks to cppcon, meetingc++ and other conferences and all C++ expert
speakers, committee members and compiler implementers for modernising C++ and
teaching it with so much enthusiasm. I had fun implementing this, hoping you
will have fun using it. Looking forward to learn more from the community.
