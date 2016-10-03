---
title:  "Easy Unary Predicates"
comments: true
categories: 
  - Algorithm
tags:
  - generic programming
  - metaprogramming
  - CRTP
  - ezl internal
  - intermediate
---

Predicates are functions that return a boolean value. Unary predicates take a
single parameter
[[drdobbs]](http://www.drdobbs.com/effective-standard-c-library-unary-predi/184403777)
[[cppref]](http://en.cppreference.com/w/cpp/concept/Predicate). They are useful
in many higher order functions e.g. '_if' algorithms in standard library such
as find_if or remove_if, partition etc.  With increase in functional style
programming and libraries within C++, predicates are going to be used more
often.

This blog post shows cute, little unary predicates that save some keystrokes
and work together well. We will walk through the implementation that begins with
thinking generic and embarks on often used metaprogramming pattern towards the end.
Let's have a look at what these predicates look like in the first place.

**Example-1**: Logical operators

{% highlight cpp %}
auto a = gt(80) || lt(40);
auto a_notone_odd = a && !eq(1) && ([](int x) { return x % 2; });

assert(a(3) == true);
assert(a_notone_odd(1) == false);
{% endhighlight %}

**Example-2**: Column selection

{% highlight cpp %}
// adding junk values to a vector
auto v = vector<tuple<int, char, int>>;
v.emplace_back(1, 'a', 11);
v.emplace_back(2, 'b', 22);
v.emplace_back(3, 'c', 33);

auto c = count_if(begin(v), end(v), lt<2>('b') || eq(3, 'c', 33));
assert(c == 2);
{% endhighlight %}

Now, let us see a close to real world example.

**Example-3**: In a unit circle

{% highlight cpp %}
// If a point is inside a circle of unit radius & center at origin
auto in_unit_circle = [] (auto a) { 
  auto x = std::get<0>(a), y = std::get<1>(a);
  return (x*x + y*y) < 1;
};

// In first quadrant and in unit circle 
// In first quadrant requires both the coordinates to be positive
auto a =  gt(0.,0.) && in_unit_circle(0., 3.);

assert(a(tuple<double, double>{0.3, 0.8}) == true);
assert(a(pair<double, double>{0.3, -0.3}) == false);

// In second, third or fourth quadrant or in unit circle
auto b = lt<1>(0.) || lt<2>(0.) || in_unit_circle(0., 3.);

auto v = vector<array<double, 2>> {% raw %}{{{% endraw %}0., .2}, {-5., 0.}, {5., 0.}};
auto c = count_if(begin(v), end(v), b);
assert(c == 2);
{% endhighlight %}


## Implementation

We begin with a simple implementation of relational operators. A relational
predicate, like equals or greater than, takes a reference value initially.
Each time the predicate is called with a parameter, it compares the parameter
value with the reference value and returns a boolean. Since, the reference
type and parameter type are not some fixed type, we express it with generic
programming as follows.

{% highlight cpp %}
template <class Ref> class Eq {
public:
  Eq(Ref r) : _ref{r} {}
  template <class T> 
  bool operator()(const T &row) { return row == _ref; }
private:
  Ref _ref;
};
{% endhighlight %}

The parameter type of the predicate method is not required to be the type of
reference (Ref). However, it is necessary that comparing the values of two
types with `==` is semantically valid. This is to say that templates have [Duck
typing](https://en.wikipedia.org/wiki/Duck_typing).

Similarly, we can have Gt and Lt classes for greater than and less than,
respectively.

Till now there is no logical operator to connect the different predicate
classes. We know that the behavior of a logical operator is same for all the
predicates that implies we should not be writing operator overloads for logical
operators separately in each predicate. Let us first try to frame a generic
logical operator.

{% highlight cpp %}
template <class Pred1, class Pred2>
class And {
public:
  And(Pred1 &&p1, Pred2 &&p2) : _pred1{forward<Pred1>(p1)}, 
                                _pred2{forward<Pred2>(p2)} {}
  template <class T> bool operator()(const T &row) {
    return _pred1(row) && _pred2(row);
  }
private:
  Pred1 _pred1;
  Pred2 _pred2;
};
{% endhighlight %}

Notice that the And class itself is a predicate (returns boolean on function
call) which is what we expect it to be. The class implementation simply says
that it can be instantiated with any two predicates and each time the
intantiated object will be called with a parameter, it will return the
conjunction of the results of the two predicates for that parameter. The
templates let us express this in a generic way with only duck-typing
rules to follow. The `&&` and `forward` are there to express that the
template predicate types can be either lvalue or rvalue and in either case
predicate values will not be copied to the class members. In former case the
member predicates (_pred1 and _pred2) reference to the constructor parameters
(p1 and p2) while in latter case the constructor parameters are moved to the
members. A simple way to understand lvalue and rvalue is that lvalue is held by
a declared variable while an rvalue is a temporary that is not assigned to any
variable. E.g. In the `a && !eq(1)` expression of Example-1, `a` is an lvalue
while `eq(1)` is an rvalue. Supporting predicates without copy is important
since predicates by definition can be non-copiable like a lambda function.

Similarly, we can create a class for other two logical operators viz. or and
not.

Now, let us create an umbrella class with all the logical operator overloads,
so that we can reuse it to add logical operator overloads to any predicate.

{% highlight cpp %}
class Logical_ops {
public:
  template <class Base, class Pred> 
  auto operator||(Base &&base, Pred &&pred) {
    return Or<Base, Pred>{forward<Base>(base), 
                          forward<Pred>(pred)};
  }
};
{% endhighlight %}

We add similar operator overloads for `&&` and `!`. 

It is however not yet clear how these pieces especially the Logical_ops class
will be used along with others. Let us tie these pieces together.

We know, inheritance gives a direct way of reusing the code. With public
inheritance we can provide the public interface of logical operator overloads
to the predicates. It looks like we need to inherit every relational operator
class from Logical_ops class. 

But, in the current form they don't fit together. The operator overload method
for a logical operator in a predicate class takes the first operand implicitly
to be `this` similar to any other method, while in current implementation of
Logical_ops we have both the operands as parameter. The first implicit operand
can vary in a restricted sense i.e. it is same as the predicate class that
inherits from Logical_ops e.g. in Eq class the type of first operand will be Eq
while the second operand can be any other predicate while in Gt the type of
first operand would only be Gt. This smells like generic programming but has a
wee bit more to it. Let us change the Logical_ops to have an implicit first
operand of any generic template class type.

{% highlight cpp %}
template <class Base> class Logical_ops {
public:
  template <class Pred> auto operator||(Pred &&pred) {
    return Or<Base, Pred>{*(Base *)this, forward<Pred>(pred)};
  }
};
{% endhighlight %}

Finally, we need to change relational predicates to inherit from Logical_ops while
passing the correct template types to it.

{% highlight cpp %}
template <class Ref> class Eq : public Logical_ops<Eq<Ref>> {
public:
  Eq(Ref r) : _ref{r} {}
  template <class T> 
  bool operator()(const T &row) { return row == _ref; }
private:
  Ref _ref;
};
{% endhighlight %}

And, same for logical predicate classes so that we can continue chaining them
with more logical operators.


{% highlight cpp %}
template <class Pred1, class Pred2>
class And : public Logical_ops<And<Pred1, Pred2>> {
public:
  And(Pred1 &&p1, Pred2 &&p2)
      : _pred1{forward<Pred1>(p1)}, _pred2{forward<Pred2>(p2)} {}
  template <class T> bool operator()(const T &row) {
    return _pred1(row) && _pred2(row);
  }
private:
  Pred1 _pred1;
  Pred2 _pred2;
};
{% endhighlight %}

The idea of passing itself as a template parameter for the base class sounds
unusual and so does its name, CRTP (curiously recurring template pattern) which in
general known as f-bounded polymorphism (even weird).

We skipped over a couple of small details like overloading the logical operators
for ltype and rtype separately, adding column selection to logical
operators etc. These details can be found in the code
[here](https://github.com/haptork/easyLambda/blob/master/include/ezl/algorithms/predicates.hpp).
It is part of the [easyLambda](https://haptork.github.io/easyLambda/) library
for data processing. Don't forget to check the library as well.

This is just one way out of many possible ways to implement such predicates.
Please suggest improvements and how you would implement this.

I am not sure how much useful these predicates are in general but I have found
them to be pretty useful for data processing. They appear in many
[examples](https://haptork.github.io/easyLambda/docs/real-world/) in
easyLambda. I believe that a library with geometrical predicates like
`in_circle` etc. would also be of use in various cases.

