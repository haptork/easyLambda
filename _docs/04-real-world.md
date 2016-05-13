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

After reduce we call the data-flow expression oueUp. This is among the expressions
that act on the data-flow. It takes us to the one unit up and the next unit we add
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

