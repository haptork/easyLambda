---
title: "Reduce Algorithms"
permalink: /docs/reduce-algo/
excerpt: "Reduce and ReduceAll Function Objects given with ezl"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

#### Reduce

Function objects available for the reduce and scan units include
count, sum, mean etc. The function objects can be called for any key
or value type. In Example--1, if it is desired to sum the numbers in
one column and all their sixth roots in another, it can be done by
adding `reduce(sum(), 0.0, 0.0)` to the dataflow. Check demoReduce for
usage examples of these function objects.


#### ReduceAll

Algorithms available for the reduceAll unit include the following. Check
demoReduceAll for usage examples of these function objects.

- summary: returns mean, stddev, min, max, median of a dataset.
- corr: correlates one column with all other columns.
- hist: bins the rows and calculates the frequency of each bin.

