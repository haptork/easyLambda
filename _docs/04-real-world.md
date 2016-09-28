---
title: "Real World Examples"
permalink: /docs/real-world/
excerpt: "Examples on real problems"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

## Word Count
Word count problem involves counting the number of times each word has occurred
in a data. Following is a complete ezl program for it.

{% highlight cpp %}
#include <string>
#include <boost/mpi.hpp>

#include <ezl/ezl.hpp>
#include <ezl/algorithms/fromFile.hpp>
#include <ezl/algorithms/reduces.hpp>

int main(int argc, char* argv[]) {
  using std::string;
  using ezl::fromFile;

  boost::mpi::environment env(argc, argv);
  assert(argc > 1);
  ezl::rise(fromFile<string>(argv[1]).rowSeparator('s').colSeparator(""))
    .reduce<1>(ezl::count(), 0).dump()
    .run();
  return 0;
}
{% endhighlight %}

We include the core ezl header, ezl header for fromFile function object and ezl
header having function objects for reduce that include count function object.
The boost mpi environment always needs to be initialized at the top.

The dataflow has rise with an ezl function object fromFile that loads
tabulated data from files. We pass the types of columns in the angular
brackets. We are expecting the file name pattern in first command line
argument. The rowSeparator property with 's' makes any white space into a new
row in the data and the whole row is a single column as column separator is an
empty string. In this way we are reading every space separated string as a
new row. Then, we have a reduce having the word as key column for grouping and
count as the function. For every word a (word, count) row is dumped on the
console. For large data we must pass 0LL or size_t(0) as the initial value of
the result to avoid overflow. We may wish to dump the results to a file by
passing filename to the dump property.

When we run this program with multiple cores, the dataflow by default runs on
all the processes available to MPI ([what is MPI]({{ base_path
}}/docs/welcome#what-is-mpi)). The riseb by default runs on all the processes
available to the dataflow and fromFile reads the file(s) in parallel by
dividing the total number of bytes equally among the processes. Since, the
reduce calculates one total count for each word, every occurrence of the word
needs to be computed by the same process. For this easyLambda sends same key
rows to the same processes using a partition function. In this way, reduce
units processes data coming from multiple processes. By default, reduce units
are given half the share of processes of parent unit. If let us say we run
with mpirun -n 4 then rise will run on all four processes and reduce will run
on two processes and all the four processes partition the data based on key
and send to these two processes. There are options to request for processes
in a different way, the defaults are set on the basis of what we found to be
reducing overall communication.

The following dataflow reduces the communication and makes the parallel
run a lot more efficient.

{% highlight cpp %}
  ezl::rise(fromFile<string>(argv[1]).rowSeparator('s').colSeparator(""))
    .reduce<1>(ezl::count(), 0).inprocess()
    .reduce<1>(ezl::sum(), 0).dump()
    .run();
{% endhighlight %}

In the dataflow there is a reduce with inprocess property. That makes the
reduce run on only the rows that are in that process. Maps are inprocess by
default.  The first reduce outputs the word, count for each word inprocess. The
secount reduce is not inprocess and hence is applied to the rows after
partitioning on word. Notice that we sum all the inprocess results to find the
final results. We have reduced the communication a lot by partitioning the
accumulated results instead of every single row. In practice we should have an
inprocess reduce whenever possible. The inprocess reduce is called a combiner
in many MapReduce like libraries.

## Monte Carlo Pi

Monte Carlo method is a randomized method which can be used to estimate value
of pi. In short, the method is to sample a number of decimal numbers uniformally
between zero and one. After sufficient number of points are samples, the number
of points falling inside the circle of unit radius are approximate value of the
area of the circle (only one quadrant) and total number of points are approximate
value of the number of points in the sqare with unit side length. The ratio of
points that are inside the circle of unit radius and total number of points is
used to estimate the value of pi. You can read about it more
[here](http://mathfaculty.fullerton.edu/mathews/n2003/montecarlopimod.html).

{% highlight cpp %}
ezl::rise(ezl::kick(trials).split())
    .map([] { 
      auto x = rand01(); // random value between 0 and 1
      auto y = rand01();
      return x*x + y*y; 
    })
    .filter(ezl::lt(1.))
    .reduce(ezl::count(), 0LL).inprocess()
    .reduce(ezl::sum(), 0LL)
    .map([trials](long long res) {
      return (4.0 * res / trials); 
    }).colsTransform().dump("", "pi in " + std::to_string(trials) + " trials:")
    .run();
{% endhighlight %}

With rise we are using kick library function object and passing an integer
number of trials to it. It calls the next unit trials number of times. The map
has a lambda function and returns the distance of a random point from the
center. Notice that the function passed in map after kick need to have no input
parameter. Next, we filter the points which lie inside the circle and count
them. The counting is done with first inprocess reduce and then with a normal
reduce to decrease communication when run on multiple processes. Next, we find
the value of pi using the ratio of points inside the circle and total points
and dump the result for display.

You can look for the MPI-only code for this
[here](https://github.com/olcf/Serial-to-Parallel--Monte-Carlo-Pi). The lines
of code in MPI-only code are far more and the algorithm is all mixed with I/O and
parallelism details. Parallelism in this way is difficult and errorsome. As you
can see the code has (had) problems as listed in this
[issue](https://github.com/olcf/Serial-to-Parallel--Monte-Carlo-Pi/issues/9).
In benchmarks section you can see that the speed of ezl code and MPI-only
optimized code is very similar.

## Csv file

Let us take an example from
[Cods2016](http://ikdd.acm.org/Site/CoDS2016/datachallenge.html). A stripped
version of original data-file is given with ezl
[here](https://github.com/haptork/easyLambda/blob/e496a3e3070b806e8c48124d3454543c4cebc9b7/data/datachallenge_cods2016/train.csv).
The data contains student profiles with scores, gender, job-salary, city etc.

The following code prints the summary of each score for male and female
separately. It writes a file "corr.txt" having correlation of English,
logical and domain scores with respect to gender. We can find similarity of
the above code with steps in spreadsheet analysis or with an SQL query. 

{% highlight cpp %}
ezl::rise(ezl::fromFile<char, std::array<float, 3>>(fileName)
            .cols({"Gender", "English", "Logical", "Domain"})
            .colSeparator("\t"))
  .filter<2>(ezl::gtAr<3>(0.F))
    .reduceAll<1>(ezl::summary()).dump("", 
      "Gender split of scores(E, L, D) resp.\n(gender|count|avg|std|min|max)")
    .oneUp()
  .map<1>([] (char gender) { 
    return float(gender == 'M' || gender == 'm');
  }).colsTransform()
  .reduceAll(ezl::corr<1>()).dump("corr.txt",
                                  "Corr. of gender vs. scores\n(gender|E|L|D)")
  .run();
{% endhighlight %}

In rise we pass an ezl function object fromFile that loads tabulated data from
files. Here, we pass the types of columns in the angular brackets. In
columns property we pass a string initializer list. The strings are the column
headers for the columns we wish to select from file. Next, we set tab character
as the separator for columns since the input file is tab separated. Notice that
gender is loaded as character denoted by 'M' or 'F' in the file and scores are
loaded into an array. In the data-file the invalid domain scores are represented
by zero value, with a filter we remove these entries. 

In filter we are using an easyLambda function object gtAr, for greater than in
arrays. In the angular brackets for gtAr we pass the array index we wish to
filter for and the reference value zero. We can select multiple array indices
with multiple reference values similar to selecting multiple columns in filter
for gt function object.

We find summary of scores for each gender that includes count, mean / average,
standard deviation, minimum value and maximum value with the library function
object summary passed to reduce having first column as key column for grouping.
Notice that summary is a generic function which can give summary of any number
of columns separately even if some of them are arrays. 

After reduce we call the dataflow expression oueUp. This is among the expressions
that act on the dataflow. It takes us to the one unit up and the next unit we add
is added not to reduce but to filter which is one level up. There is nothing added
to reduce branch and the following units are added to prior unit filter.

With next map we change the gender column to float 0.0 for female and 1.0 for male.
The selected input column is transformed in place using colsTransform property. We
then add a reduceAll without any grouping and pass a library function object corr.
The one index in the corr function tells it the column with respect to which we 
want to find correlations of all other columns. We find correlation of gender with
all the scores. Again, corr is a generic function which can take any number of
columns that can also be an array.

fromFile function that is used for loading the file data has quite
comprehensive set of properties to configure. It takes care of errors in
reading, reads a single file or multiple files in parallel, has options for
imposing strict schema that rejects rows having different number of columns or
loose schema that fills in defaults if columns are less or ignores extra
columns. For more information on its properties please check references section.

We select columns that we want to transform and use `colsTransform()`
to do it in-place. We reduce to find aggregate values of multiple rows like
correlation and summary. The summary for male and female are found separately
by selecting column 1st as grouping key in the reduce. A different analysis of
the same data is given in [cods2016.cpp example](../../examples/cods2016.cpp).

## LAMMPS Simulation (Interstitial Count)

[LAMMPS](http://lammps.sandia.gov/) is a molecular dynamics simulation
software. It can deal with very large systems using MPI. While the LAMMPS can
scale fairly well on clusters the post-processors written to work with the
simulation results may not.  The general output from LAMMPS is timestep
followed by atomic co-ordinates and other atomic properties like type of atom,
id etc for the time-step. ezl fromFile function object has a property lammps
which gives each atom with time-step value in every row. It also enables safe
parallel reading from a single dump or multiple files.

In the following dataflow. We find the number of self-interstitials in every
time-step. A self-interstitial is an atom of the same type as the lattice type
but it is sufficiently far from any of the lattice site in the perfect crystal
structure.

{% highlight cpp %}
double calcOffset(std::array<float, 3> c);

ezl::rise(ezl::fromFile<int, std::array<float, 3>>(argv[1])
         .lammps().cols({6, 3, 4, 5}))
    .map<2>(calcOffset).colsTransform()
    .filter<2>(ezl::gt(threshold))
    .reduce<1>(ezl::count(), 0).inprocess().ordered()
    .reduceAll([](vector<tuple<int, int>> a) {
      std::sort(a.begin(), a.end());
      return a;
    }).dump(outFile)
    .run();
{% endhighlight %}

With rise, we use fromFile and provide column types. The columns selected with
cols are time-step and x, y, z co-ordinates. We are expecting to have file name
pattern in the first argument. The property lammps is used to make sure that
time-step is added to every row. The calcOffset function finds the distance of
the atom from its nearest lattice site based on the lattice structure [more]().
We have omitted the implementation details of this function for simplicity. We
can use it as a black--box for calculating offset of an atom given its
coordinates just by knowing the function signature.

After finding the offset we filter the atoms that have offset greater than a
given threshold. These are the interstitial atoms. As a side effect of having
the time-step at the top of all the rows for the time-step, after using lammps
property for reading it is certain that all the atoms for a single time-step
are read by the same process. Hence, a single inprocess reduce is sufficient
for reduce having time-step as one of the key column. This also implies no
communication is required for reduction. The ordered() property is optional
here. It makes sure that the reduce results are streamed as soon as the current
key changes. Since, we know that all the time-step rows are going to be
adjacent to each other, it makes sense to stream the result as soon as the last
row of a time-step is recieved by the reduce. At the end, we sort the vector of
timestep, interstitial-count rows using a reduceAll without any key.

Notice that we can post-process simulation results where in a time-step we have
more atoms that can fit the memory. Since, the operations are streaming. The
fromFile function object returns a single row it finds, which streams to map and
then filter. If it passes the predicate of filter, it moves to reduce. The row
is passed to the function object and result is updated for the key. If it
has a new key than the prior key and its result also get streamed to the last
reduceAll since we are using ordered property.

## Logistic Regression

It is a popular classification algorithm in machine learning. The logistic
regression training is done using iterative gradient descent. We are showing
training as well as testing dataflows for it.

{% highlight cpp %}
  auto reader = ezl::fromFile<double, array<double, dim>>(argv[1])
                  .colSeparator(",");

  // load once in memory
  auto trainData = ezl::rise(reader)
                  .get();
{% endhighlight %}

The reader is a fromFile function object. The first command line argument has
the training data file. The training data has first column as result and next
`dim` number of columns as input variables.

After running the dataflow with get, the trainData in different
processes has different rows loaded from the files, almost equally shared. The
type of trainData is vector<tuple<double, array<double,dim>> that is same as
vector of the output columns of the unit on which get is called on. We
can use trainData with raw MPI, any other library or user code. Each process
will work on its share of data.

{% highlight cpp %}
  array<double, dim> w{};
  auto trainFlow = ezl::rise(ezl::fromMem(trainData))
                     .map([&w](auto& y, auto& x) {
                       return calcGrad(y, x, w);    
                     }).colsTransform()
                     .reduce(sumArray, array<double, dim>{}).inprocess()
                     .reduce(sumArray, array<double, dim>{})
                     .map(updateWeights).colsResult()
                     .map(calcNorm(w))
                       .prll(1.0, ezl::llmode::task | ezl::llmode::all)
                     .build();
{% endhighlight %}

The weights `w` are initialized to zero with `{}` for initialization.
The fromMem function in the rise unit uses trainData to stream rows in the data flow.
Next, we calculate gradient array from each row. The weights are captured in the lambda
function. The summation of gradient arrays is carried out in the reduce step,
first for inprocess data and then globally to minimize communication. The
upddateWeights map outputs updated weights. The calcNorm map calculates norm as a
measure of change from prior weights to new weigths. 

We are changing the prll (parallel) property of the calcNorm map. By default a
map runs in-process with the source unit. It gets all the rows that are there
in the process and processes them without any communication involved. The
reduce units by default run on half the share of processes available to their
source unit. The data is partitioned according to the key. However, for a
reduce without key all the rows have to reach a single process for reduction so
it executes in a single process. 

Here, calcnorm map executes in all the processes available to the dataflow since
we request 1.0 ratio of processes with llmode::task mode. Without the task mode it would
be 1.0 or all the processes of the source unit rather than the dataflow. By default the
rows are split between the number of processes in a round robin fashion, however with
llmode::all we make sure that that all the maps in different processes get all the rows.


{% highlight cpp %}
  auto norm = epsilon * 2;
  while (norm < epsilon) {
    array<double, dim> wn;
    tie(wn, norm) = ezl::flow(trainFlow).get()[0];
    w = move(wn);
  }
{% endhighlight %}

In the above loop we keep running the training dataflow with updated weights
as long as the norm is more than a given epsilon.

{% highlight cpp %}
  auto testFlow = ezl::rise(reader)
                      .map<2>([&w](const auto& x) {
                        auto pred = 0.;
                        for (size_t i = 0; i < get<0>(x).size(); ++i) {
                          pred += w[i] * get<0>(x)[i];
                        }
                        return (sigmoid(pred) > 0.5);
                      }).colsTransform()
                      .reduce<1, 2>(ezl::count(), 0)
                        .dump("", "real-y, predicted-y, count")
                      .build();

  for (int i = 1; i < argc; ++i) {
    reader = reader.filePattern(argv[i]);
    cout<<"Testing for "<<argv[i]<<endl;
    ezl::flow(testFlow).run();
  }
{% endhighlight %}

We use the final weights from training to test the data-sets given as file
patterns in command line arguments two onwards.

The ezl::rise unit uses the same reader as the training dataflow. The map
that follows takes the input features and uses the weights to predict the
outcome. The reduce calculates the count for each real and predicted outcome
pair.

After building the dataflow, we run it for different test data-sets by changing
the input file pattern of the reader and running the dataflow.

The example demonstrates various useful features, how can the data be loaded into
memory once and then can be processed multiple times with easyLambda dataflow
or used elsewhere. It also utilizes the fact that easyLambda dataflow columns
at any step are immutable, hence the input data can be used multiple times and
is never modified. EasyLambda however makes sure that the copies are created
only when required and uses const references for column selection etc. It shows
the use of prll property to spread the data on all processes and how that can
be used to use the data outside the easyLambda dataflow.
