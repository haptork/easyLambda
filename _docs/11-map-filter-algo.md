---
title: "Map and Filter Algorithms"
permalink: /docs/map-filter-algo/
excerpt: "Map and Filter Function Objects given with ezl"
---
{% include base_path %}
{% include toc icon="gears" title="Contents" %}

Easylambda's predicate function objects consist of logical operations like
`eq` (equals to), `gt` (greater than) etc. These can be combined
with the relational operators `&&`, `||` or `!`.  The predicates can take
multiple values for comparison with multiple columns e.g.
`filter<3,2>(gt(7.0, 4))` filters the rows with third column greater than
$7.0$ and second column greater than $4$. The logical function objects
have column selection for a single column e.g. `gt<3>(7.0) && !et<1>('a')`
creates a predicate that checks for third column be greater than 7.0 and
the first column not be equal to 'a'.

Special algorithms for merging array columns and for exploding arrays are
available to the map unit. In addition, the map unit can generally use
algorithms from the C++ standard library.

Following are the function object names available. Check demoMapFilter for
usage examples of these function objects.

#### Map

- mergeAr

- explodeAr

- addSerialNo

#### Filter

- eq
- neq
- gt
- lt
- eqAr
- neqAr
- gtAr
- ltAr
- tautology
