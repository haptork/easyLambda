---
title: "Analyzing Data Files"
permalink: /docs/data-files/
excerpt: "Example on data-file"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

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


