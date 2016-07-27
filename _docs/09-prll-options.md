---
title: "Parallel property"
permalink: /docs/prll-options/
excerpt: "Refernce on prll property"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

A data--flow enables task parallelism, inherently. Different units in a linear
data--flow can run on different processes in a data--pipeline. The branches in
a data--flow make total ordering of computation into partial ordering. The two
branches can run in separate processes and their results can be merged later in
the data--flow if needed. In practical MPI applications task parallelism is
beneficial only if the computation on each process is intensive compared to
the relatively higher communication costs of message passing.

The map and reduce enable data parallelism. A single map unit can be made to
run on separate processes on different rows simultaneously. A reduce is
parallel on different groups of rows, however it requires that all the rows in
a group be communicated to the same processor from the map processes.

The easyLambda makes good use of task--parallelism and data--parallelism with 
optimum defaults for MPI. Every unit has `.prll()` property that can 
be used to override the defaults if required. The following figure shows the
overview of parallel options for units in a data--flow. 

![Parallel expressions]({{ site.url }}{{ site.baseurl }}/images/prll.png)

In the above figure, first unit can be a rise running on {0, 1} process
ranks, {2,3,4} can be running a map or reduce and so on. It can be seen that a
reduce unit is by default parallel and map units are by default in-process.
This is because a reduce requires partitioning of rows on key columns i.e. all
the rows belonging to the same group need to be communicated to the same
process. The reduce units can be made to run in-process with
`inprocess()` property.

The defaults are that a data-flow has all the processes available to MPI
environment, rise has all the processes available to the data-flow, maps and
filters run on same processes as their source, reduce and reduceAll run in
parallel, on half the processes of their source. This configuration decreases
the communication cost and scales well for different problems. We will see the
benchmarks for different problems with these defaults below in benchmarks.

If for a map or filter prll property is used without the key columns,
the input data rows are, by default, shared among the processes in a
round-robin fashion. This can be used in sharding like operations or to spread
input data coming from a single stream to a number of nodes. With `duplicateOnAll`
mode the input data rows can be duplicated to all the processes running the
unit. 

Some patterns that are used frequently can be formed by a combination of these
parallel properties, such as combiner and broadcast. 

An in-process reduce is equivalent to what is termed as combiner in basic
MapReduce model. For parallel efficiency, it is important to have an
in-process reduction before reducing across processes to decrease overall
communication. 

Broadcast function in MPI makes the data available to all the processes. In
easyLambda same can be achieved by having `duplicateOnAll` prll mode along with the
task mode. It can be used to communicate the results to all the processes at
certain step in the computation or to have the result with `runResult`
available on all the processes after the data--flow execution finishes. The data
then can be used outside MPI as required.


Check demoPrll for usage. The following are the properties that can be
added to any unit to change its parallel behaviour.

- prll

With `prll(...)` property, the processes can be requested by number of
processes, ratio of processes of source / parent unit, or exact rank of
processes. If the requested processes are not available then also the program
runs correctly with best possible allocation to the unit.

The prll property has mode as the second argument. A parallel map or
reduce without task mode can only be allocated with processes from the set of
processes of its source / parent. This also implies that without task mode, a
unit does not have more processes than its source. However, in task mode a unit
gets processes directly from the share of processes available to the data-flow.
The `run(...)` expression also has optional parameter for process request
that limits the processes allocated to the data-flow, itself. This process request
can also be ratio of total processes, exact number of processes or ranks of
processes (passed as initializer_list<int> or vector<int>).

- hash

The data in reduce is partitioned on key columns. A map unit can also have
partitioning of data on keys specified along with `prll<...>()` property
with key column indices inside angular brackets. The default partitioning
function used is the hash function on keys. The function can be overridden by
`hash(...)` property.

Check example interstitial-count or demoPrll for usage.

- inprocess
An inprocess unit receives data only from its parent. Map and filter are by
default inprocess. Reduce and zip are not and recieve all the data for particular
keys from all the processes. However, it is more efficient to reduce the data inprocess
and then reduce the overall results. `inprocess()` property can be added to any unit
to make it process data of its own process only.

- ordered
If the unit has keys for data partitioning and the unit is not inprocess then adding
ordering keeps the data from each process in order and the unit gets the data from
one process in order all at the same time. This may be required for calculations that
depend on ordering.
