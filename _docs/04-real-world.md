---
title: "Real World Examples"
permalink: /docs/real-world/
excerpt: "Examples on real problems"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

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
ezl::rise(ezl::fromFile<char, array<float, 3>>(fileName)
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
files. It has quite comprehensive set of properties to configure. It takes care
of errors in reading, reads a single file or multiple files in parallel, has
options for imposing strict schema that rejects rows having different number of
columns or loose schema that fills in defaults if columns are less or ignores
extra columns. Here, we select the columns on which we want to apply filter
criteria. We select columns that we want to transform and use `colsTransform()`
to do it in-place. We reduce to find aggregate values of multiple rows like
correlation and summary. The summary for male and female are found separately
by selecting column 1st as grouping key in the reduce. A different analysis of
the same data is given in [cods2016.cpp example](../../examples/cods2016.cpp).


