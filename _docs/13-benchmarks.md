---
title: "Benchmarks"
permalink: /docs/benchmarks/
excerpt: "Benchmarks for performance and ease of use"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

EasyLambda aims to provide a high level parallel programming abstraction while
incurring minimal overheads. Whether you are writing parallel code for a multicore
machine or hundreds of distributed nodes, easyLambda lets you ignore the idiosyncrasises
of configuring parallelism on these diverse platforms. The code written for a few
cores on a single machine scales appropriately when a multi-node distributed cluster
is available. Through the use of modern C++ features, it is expressive and succinct. 
It is efficient in its parallelism because it uses MPI as its backend. 
[[1]](http://www.sciencedirect.com/science/article/pii/S1877050915017895).

This section demonstrates performance and ease of programming with easyLambda
with the help of experiments carried out on an HPC cluster, cloud cluster and
multi core machine.

## Performance
The benchmarking results of easyLambda programs for the following problems are
presented.

- **wordcount**: 
The wordcount problem involves a file read followed by a partitioned reduce. The partitioned
reduce imposes inter-process communications on the problem. The example code for wordcount 
problem are easy to find for any MapReduce like library that implements a partitioned reduce.
[[code]](https://github.com/haptork/easyLambda/tree/master/examples/wordcount.cpp)
[[walkthrough]]({{ base_path }}/docs/real-world#word-count)

- **logreg**: 
A logistic regression analysis trained with a stochastic gradient is an iterative algorithm.
It reads in the data once and then iterates over the data to improve the weights. The iterations 
involve calculation of the gradient which is a linear operation over the number of dimensions of 
the input. Each iteration involves inter-process communications. Classic MapReduce frameworks 
such as Hadoop involve disk operations for each iteration making these iterative algorithms slow. 
Newer frameworks like Spark improve this aspect of performance. 

[[code]](https://github.com/haptork/easyLambda/tree/master/examples/logreg.cpp)
[[walkthrough]]({{ base_path }}/docs/real-world#logistic-regression)

- **pi**: 
The Monte Carlo example to generate the digits of pi does not access the filesystem. It only
requires all communications to be directed to a single process. Since, the code does not involve
a partitioned reduce, it can be implemented with a raw MPI reduce.

[[code]](https://github.com/haptork/easyLambda/tree/master/examples/pi.cpp)
[[walkthrough]]({{ base_path }}/docs/real-world#monte-carlo-pi)

- **heat**: 
The solution for this problem provides an explicit finite difference solution for a one 
dimensional heat equation. It requires filesystem writes in order to write the results 
of the cells/grid. During each iteration, each process communicates its rows to its 
two adjacent processes. 

[[code]](https://github.com/haptork/easyLambda/tree/master/examples/1d-Diffusion.cpp)

The following figure shows easyLambda vs Spark execution times for the problems on 
different number of processes. The codes were executed on amazon's elastic cloud cluster 
(EC2) with an m3.2xlarge instance type. To use easyLambda on EC2, a StarCluster with 
NFSv3 filesystem was used. Spark uses HDFS as a filesystem that is deployed by standard 
spark-ec2 scripts. In all of the Spark vs easyLambda benchmarks, easyLambda invariably 
performs around an order of magnitude better. However, Spark provides fault tolerance 
and better data handling features that easyLambda currently lacks. There are also 
differences in the basic philosophy, target community and use cases of the two.

The codes are taken from the documented examples supplied with these libraries except
for the problem 'heat' in Spark. For this problem, it was necessary to write the Spark
program myself. To avoid accusations of deliberately trying to make Spark look bad, the 
Spark and easyLambda programs are written so as to be isomorphic and as close to each 
other as possible. 

<figure>
  <img src="{{ site.url }}{{ site.baseurl }}/images/benchelastic.png" alt="benchelastic">
  <figcaption>
    The execution times of easyLambda and Spark codes on amazon elastic butt
    cluster for various problems. The pi problem is run for 1e10 trials, while
    heat problem is run for 10 iterations with 4e7 cells, wordcount has data
    size of 3.2GB while logreg has data size of 2.2GB and is run for 10
    iterations.
  </figcaption>
</figure>

The following table shows benchmark results for the problems executed on a
Linux cluster with NFSv3 file-system over TCP. The problems scale nearly
linear when the problem size is sufficient compared to number of processes.
Since, similar code for pi problem can be written using raw MPI in a straight
forward way using `MPI_Reduce`, it is compared with pi implementation of
easyLambda. The raw MPI code (pi-MPI) and easyLambda code (pi-ezl) have
similar performance.


processes  | 12 | 24 | 48 | 96 | data
------ |---------|----|----|-----
pi-ezl | 48s | 55s| 58s| 58s| weak
pi-MPI | 46s | 54s| 58s| 59s| weak
trials(1e11)| 0.125| 0.25 | 0.5 | 1 | weak
----   | ---       |
wordcount | 178s| 114s| 82s| 80s| 12.5GB
logreg | 190s| 91s| 50s| 36s| 2.9GB
heat | 300s|156s | 81s| 42s| 1e8 pts

Time of execution is in seconds for different problems. Weak scaling is used
for pi with number of trials given below the execution times. 

The following table shows benchmarks for logreg problem with more number of processes 
and bigger data sizes on a Linux cluster with Lustre file-system over RDMA. Over RDMA 
the wordcount with similar data takes less than 20 seconds for lowest number of processes 
viz. 24 and reduces to around 10 seconds for 384 processes. The pi problem does not benefit
from the file-system and shows similar performance as in NFSv3 cluster.

processes| 24 | 48 | 96 | 192| 384| data
------   |---------|----|----|-----
logreg   | 336s| 187s| 100s| 55s| 30s| 48GB
logreg   | 23s| 24s| 26s| 27s| 30s| weak
data(GB) | 3 | 6 | 12 | 24 | 48 | -

EasyLambda scales well on multi-cores as shown in the following table. The 
performance is compared with MR-MPI library. The code for the wordcount problem in
MR-MPI library is taken from its examples.

processes  | 1 | 2 | 4 | data
------ |---------|----|----|-----
wordcount-ezl | 27s| 15.5s| 12.4s| 1200MB
wordcount-MRMPI | 27s |34s | 37s| 1200MB
logreg. | 120s |63s | 38s| 450MB
pi-MC | 111s |56s | 39s| 4x10^9trials

Other problems like post processing atomic simulations, machine learning on
images with high dimensional features show similar scaling trends. However,
with higher dimensional matrices in machine learning the cache effects make
the benchmarks little fluctuating, but the overall scaling remains same. The
current logistic regression uses vectorized simd operations for multiplications
when compiled with optimization flag. The openMP thread model does not give as
good performance as auto vectorization. Other libraries can be used along with
easyLambda library for heterogeneous parallelism.

## Ease of Programming

The approximate lines of user code for the implementation of the problems in
different parallel languages and libraries is shown in the following figure.
The codes whenever available, are taken from the example codes of the
libraries. The language and platform specific lines that are not related to
the problem are not counted.

<figure>
  <img src="{{ site.url }}{{ site.baseurl }}/images/benchloc.png" alt="benchloc">
  <figcaption>
    The approximate number of lines of code for various problems in different
    languages and libraries.  
  </figcaption>
</figure>

Arguably, the number of lines of code required to write a program is a decent
indicator of the readability and maintainability of that code. 

## Versatility

easyLambda library has been used for training & testing image classifiers
in parallel. It has been used with libraries like openCV, Dlib, tiny-dnn etc.
Besides data analytics and machine learning it has also been used to create
post-processors for scientific computation with multiple reusable dataflows. 

#### Acknowledgements

I wish to thank [eicossa](https://github.com/eicossa) and Nitesh for their
continuous help in pulling this through.
