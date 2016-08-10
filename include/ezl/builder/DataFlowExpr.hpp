/*!
 * @file
 * class DataFlowExpr
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef DATAFLOWEXPR_EZL_H
#define DATAFLOWEXPR_EZL_H

#include <functional>

#include <boost/functional/hash.hpp>

#include <ezl/algorithms/io.hpp>

#include <ezl/pipeline/Flow.hpp>
#include <ezl/builder/DumpExpr.hpp>
#include <ezl/builder/FilterBuilder.hpp>
#include <ezl/builder/LoadUnitBuilder.hpp>
#include <ezl/builder/MapBuilder.hpp>
#include <ezl/builder/PrllExpr.hpp>
#include <ezl/builder/ReduceAllBuilder.hpp>
#include <ezl/builder/ReduceBuilder.hpp>
#include <ezl/builder/ZipBuilder.hpp>

#include <ezl/helper/Karta.hpp>
#include <ezl/helper/meta/slctTuple.hpp>
#include <ezl/helper/meta/slct.hpp>
#include <ezl/helper/meta/typeInfo.hpp>

namespace ezl {
namespace detail {

/*!
 * @ingroup builder
 * DataFlow expressions for appending new units, building/running current
 * pipeline, adding child or moving up after building current.
 *
 * T is CRTP parent type.
 * */
template <class T, class Fl> struct DataFlowExpr {
  using emptyHash = boost::hash<std::tuple<>>;
protected:
  DataFlowExpr() = default;
public:
  // for adding map unit e.g. map([](int x) { return x * 2; })
  template <class F> auto map(F&& f) {
    auto curUnit = ((T *)this)->_buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    using O = typename meta::MapDefTypesNoSlct<I, F>::odefslct;
    return MapBuilder<I, nomask, F, O, meta::slct<>, emptyHash, Fl>{
      std::forward<F>(f), curUnit, _fl};
  }
  // for adding map unit with input column selection
  // e.g. map<3,1>([](int x, float y) { return y / x; })
  template <int N, int... Ns, class F> auto map(F&& f) {
    auto curUnit = ((T *)this)->_buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    using S = meta::saneSlct<std::tuple_size<I>::value, N, Ns...>;
    using O = typename meta::MapDefTypes<I, S, F>::odefslct;
    return MapBuilder<I, S, F, O, meta::slct<>, emptyHash, Fl>{
      std::forward<F>(f), curUnit, _fl};
  }
  // for adding filter unit e.g. filter(eq(0, 'e') || gt<1>(5))
  template <class F> auto filter(F&& f) {
    auto curUnit = ((T *)this)->_buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    return FilterBuilder<I, nomask, F, nomask, meta::slct<>, emptyHash, Fl>{
        std::forward<F>(f), curUnit, _fl};
  }
  // for adding filter unit with input column selection
  // e.g. filter<3,1>(eq(0, 'e') || gt<1>(5))
  template <int N, int... Ns, class F> auto filter(F&& f) {
    auto curUnit = ((T *)this)->_buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    using S = meta::saneSlct<std::tuple_size<I>::value, N, Ns...>;
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    return FilterBuilder<I, S, F, nomask, meta::slct<>, emptyHash, 
                         Fl>{std::forward<F>(f), curUnit, _fl};
  }
  // zip with two different keys e.g. zip<key<4, 1>, key<3, 1>>(pipe)
  template <class K1, class K2, class Pre> auto zip(Pre pre) {
    auto curUnit = ((T *)this)->_buildUnit();
    using I1 = typename decltype(curUnit)::element_type::otype;
    using I2 = typename decltype(pre)::element_type::otype;
    constexpr auto size1 = std::tuple_size<I1>::value;
    constexpr auto size2 = std::tuple_size<I2>::value;
    using saneK1 = typename meta::saneSlctImpl<size1, K1>::type;
    using saneK2 = typename meta::saneSlctImpl<size2, K2>::type;
    using nomask = typename meta::fillSlct<0, size1 + size2>::type;
    return _zipImpl<saneK1, saneK2, nomask, decltype(curUnit), Pre>
      (curUnit, pre, std::is_same<I1, I2>());
  }
  // zip with single key for both e.g. zip<4, 1>(pipe)
  template <int... Ks, class Pre2> auto zip(Pre2 pre2) {
    return zip<meta::slct<Ks...>, meta::slct<Ks...>, Pre2>(pre2);
  }
  // reduce with key, value selection & multiple outputs
  // e.g. reduce<key<3,1>, val<4,2>>(sum(), 0.0, 0)
  template <class K, class V, class F, class FO, class... FOs>
  auto reduce(F&& f, FO&& fo, FOs&&... fos) {
    auto curUnit = ((T *)this)->_buildUnit();
    using KV = _reduceKeyValue<decltype(curUnit), K, V>;
    return _reduceMultiHelper<typename KV::saneK, typename KV::saneV>(curUnit, std::forward<F>(f),
      std::make_tuple(std::forward<FO>(fo), std::forward<FOs>(fos)...));
  }
  // reduce with key, value selection and single output
  // e.g. reduce<key<3,1>, val<2>>(count(), 0
  template <class K, class V, class F, class FO>
  auto reduce(F&& f, FO&& fo) {
    auto curUnit = ((T *)this)->_buildUnit();
    using KV = _reduceKeyValue<decltype(curUnit), K, V>;  
    return _reduceSingleHelper<typename KV::saneK, typename KV::saneV>(curUnit, std::forward<F>(f),
      std::forward<FO>(fo));
  }
  // reduce for only key and multiple outputs
  // e.g. reduce<3,1>(sum(), 0.0, 0)
  template <int... Ns, class F, class FO, class... FOs>
  auto reduce(F&& f, FO&& fo, FOs&&... fos) {
    auto curUnit = ((T *)this)->_buildUnit();
    using KV = _reduceOnlyKey<decltype(curUnit), Ns...>;
    return _reduceMultiHelper<typename KV::saneK, typename KV::saneV>(curUnit, std::forward<F>(f),
      std::make_tuple(std::forward<FO>(fo), std::forward<FOs>(fos)...));
  }
  // reduce for only key and single output e.g. reduce<3,1>(count(), 0)
  template <int... Ns, class F, class FO>
  auto reduce(F&& f, FO&& fo) {
    auto curUnit = ((T *)this)->_buildUnit();
    using KV = _reduceOnlyKey<decltype(curUnit), Ns...>;
    return _reduceSingleHelper<typename KV::saneK, typename KV::saneV>(curUnit, std::forward<F>(f),
      std::forward<FO>(fo));
  }
  // reduceAll for only key e.g. reduceAll<3,1>(summary())
  template <int... Ns, class F> auto reduceAll(F&& f) {
    auto curUnit = ((T *)this)->_buildUnit();
    using KV = _reduceOnlyKey<decltype(curUnit), Ns...>;
    return _reduceAllHelper<typename KV::saneK, typename KV::saneV>(curUnit, std::forward<F>(f));
  }
  // reduceAll for key and value e.g. reduceAll<key<3,1>, val<2>>(summary())
  template <class K, class V, class F> auto reduceAll(F&& f) {
    auto curUnit = ((T *)this)->_buildUnit();
    using KV = _reduceKeyValue<decltype(curUnit), K, V>;
    return _reduceAllHelper<typename KV::saneK, typename KV::saneV>(curUnit, std::forward<F>(f));
  }
  // builds the flow and return it for later use
  // @return shared_ptr<Flow<tuple<Is...>, tuple<Os...>>> Flow of tuple of
  //   input columns and tuple of output columns. Input columns are the input
  //   columns of first unit and output columns are from the stream of last /
  //   current unit. If rise is the first unit then input is of type nullptr_t     
  auto build() {
    auto curUnit = ((T *)this)->_buildUnit();
    using O = typename decltype(curUnit)::element_type::otype;
    auto fl = std::make_shared<Flow<Fl, O>>(_fl); 
    fl->addLast(curUnit);
    return fl;
  }
  // builds the flow, runs it and return it for later use. If there is no
  // rise unit attached to the flow then running has no effect.
  // @param procs Process request in terms of exact number of processes,
  //    ratio of total processes or exact rank of processes in a vector<int>.
  // @return same as build
  template <class Ptype> auto run(Ptype procs, bool refresh = true) {
    auto fl = build();
    if (refresh) Karta::inst().refresh();
    if (fl) Karta::inst().run(fl.get(), ProcReq{procs});
    return fl;
  }
  // builds the flow, runs it and return it for later use. If there is no
  // rise unit attached to the flow then running has no effect.
  // @param lprocs optional process request as exact ranks in initializer_list<int>
  // @return same as build
  auto run(std::initializer_list<int> lprocs = {}, bool refresh = true) {
    std::vector<int> procs(std::begin(lprocs), std::end(lprocs));
    return run(procs, refresh);
  }

  // builds the flow, runs it and returns the output rows. If there is no
  // rise unit attached to the flow then running has no effect.
  // @param procs Process request in terms of exact number of processes,
  //    ratio of total processes or exact rank of processes in a vector<int>.
  // @return returns the vector of tuple of output column types.
  template <class Ptype> auto runResult(Ptype procs, bool refresh = true) {
    auto fl = build();
    using I = typename decltype(fl)::element_type::otype;
    std::vector<typename meta::SlctTupleType<I>::type> buffer;
    if (fl->isEmpty()) return buffer;
    auto dumper = dumpMem(buffer);
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    auto dumpfl =
        std::make_shared<Filter<I, nomask, decltype(dumper), nomask>>(dumper);
    fl->next(dumpfl, fl);
    if (refresh) Karta::inst().refresh();
    Karta::inst().run(dumpfl.get(), ProcReq{procs});
    fl->unNext(dumpfl.get());
    return buffer;
  }

  // builds the flow, runs it and returns the output rows. If there is no
  // rise unit attached to the flow then running has no effect.
  // @param lprocs optional process request as exact ranks in initializer_list<int>
  // @return returns the vector of tuple of output column types.
  auto runResult(std::initializer_list<int> lprocs = {}, bool refresh = true) {
    std::vector<int> procs(std::begin(lprocs), std::end(lprocs));
    return runResult(procs, refresh);
  }

  // builds the current unit but as a branch, any expression that appears after is
  // called on one prior unit. e.g. if a new unit is added afterwards it gets
  // output data stream from the one prior unit. Generally usuful for aggregating
  // and viewing intermediate results for verification etc. and then continuing
  // with the main logic.
  //                           --> | cur unit with oneUp() |
  //                           |
  // | cur dataflow | --> | prior unit | --> | next unit / expression |
  auto oneUp() {
    auto pre = ((T *)this)->prev();
    auto curUnit = ((T *)this)->_buildUnit();
    using I = typename decltype(pre)::element_type;
    return LoadUnitBuilder<I, Fl> {pre, _fl};
  }

  // builds the current unit & adds a flow as a branch that has input type same
  // as output of the current unit. The dataflow continues from the current
  // unit in the trunk dataflow. Any expression afterwards is applicable to the
  // current unit e.g. if a new unit is added afterwards it gets output data
  // stream from the current dataflow.
  //                           --> | branch flow within .branchFlow(...) |
  //                           |
  // | cur dataflow | --> | cur unit | --> | next unit / expression |
  template <class I>
  auto branchFlow(I nx) {
    auto curUnit = ((T *)this)->_buildUnit();
    nx->prev(curUnit, nx);
    return LoadUnitBuilder<typename decltype(curUnit)::element_type, Fl>{curUnit, _fl};
  }

  // builds the current unit & adds a flow to the trunk that has input type same
  // as output of the current unit. The dataflow continues from the flow added.
  // Any expression afterwards is applicable to the flow just added e.g. if a
  // new unit is added afterwards it gets output data stream from the new flow.
  // | cur dataflow | -->| cur unit | -->| added flow | --> | next unit / expr |
  template <class I>
  auto addFlow(I nx) {
    auto curUnit = ((T *)this)->_buildUnit();
    nx->prev(curUnit, nx);
    return LoadUnitBuilder<typename I::element_type, Fl>{nx, _fl};
  }

  // builds the current unit, takes another flow with same output stream and 
  // merges the output stream of both such that the next unit gets output from both.
  // Similar to concatenate in terms of list.
  // | cur dataflow | --> | cur unit |----> | next unit / expression |
  //                                     ^
  //                                     |
  //         | flow inside merge(...) |--
  template <class I>
  auto merge(I pr) {
    auto curUnit = ((T *)this)->_buildUnit();
    auto src = std::make_shared<ezl::Flow<std::nullptr_t, typename I::element_type::otype>>();
    src->addLast(curUnit);
    src->addLast(pr);
    return LoadUnitBuilder<typename I::element_type, Fl>{src, _fl};
  }

protected:
  Flow<Fl, std::nullptr_t> _fl;

private:
  // zip for different types of sources
  template <class K1, class K2, class nomask, class Cur, class Pre>
  auto _zipImpl(Cur curUnit, Pre pre, std::false_type) {
    using I = typename Cur::element_type::otype;
    using ktype = typename meta::SlctTupleRefType<I, K1>::type;
    return ZipBuilder<I, typename Pre::element_type::otype, K1, K2, nomask,
                      boost::hash<std::tuple<ktype>>, Fl>{curUnit, pre, _fl};
  }
  // zip for same types of sources
  template <class K1, class K2, class nomask, class Cur, class Pre>
  auto _zipImpl(Cur curUnit, Pre pre, std::true_type) {
    using I = typename Cur::element_type::otype;
    constexpr static auto size = std::tuple_size<I>::value;
    using S = typename meta::fillSlct<0, size>::type;
    using O = typename meta::ConcatenateSlct<S, meta::slct<size+1>>::type;
    auto func = [](const auto&) { return true; };
    auto mapfl = std::make_shared<Map<meta::MapTypes<I, S, decltype(func), O>>>(std::move(func));
    pre->next(mapfl, pre);
    return _zipImpl<K1, K2, nomask, Cur, decltype(mapfl)>(curUnit, mapfl, std::false_type{});
  }
  // reduce with single output which can be a vector
  template <class K, class V, class Cur, class F, class FO>
  auto _reduceSingleHelper(Cur curUnit, F&& f, FO&& fo) {
    using I = typename Cur::element_type::otype;
    using types = meta::ReduceDefTypes<I, K, std::tuple<FO>>;
    return ReduceBuilder<I, V, F, FO, typename types::odefslct, K,
                         boost::hash<typename types::ktype>, Fl>{
        std::forward<F>(f), std::forward<FO>(fo), curUnit, _fl, false};
  }
  // reduce with multiple outputs which can be a vector
  template <class K, class V, class Cur, class F, class FO>
  auto _reduceMultiHelper(Cur curUnit, F&& f, FO&& fo) {
    using I = typename Cur::element_type::otype;
    using types = meta::ReduceDefTypes<I, K, FO>;
    return ReduceBuilder<I, V, F, FO, typename types::odefslct, K,
                         boost::hash<typename types::ktype>, Fl>{
      std::forward<F>(f), std::forward<FO>(fo), curUnit, _fl, false};
  }
  // reduce types for given key value
  template <class Cur, class K, class V>
  struct _reduceKeyValue {
    constexpr static auto size = std::tuple_size<typename Cur::element_type::otype>::value;
    using saneK = typename meta::saneSlctImpl<size, K>::type;
    using saneV = typename meta::saneSlctImpl<size, V>::type;
  };
  // reduce types for given key only
  template <class Cur, int... Ks>
  struct _reduceOnlyKey {
    constexpr static auto size = std::tuple_size<typename Cur::element_type::otype>::value;
    using saneK = meta::saneSlct<size, Ks...>;
    using saneV = meta::InvertSlct<size, saneK>;
  };
  // reduceAll helper
  template <class K, class V, class Cur, class F> auto _reduceAllHelper(Cur& curUnit, F&& f) {
    using I = typename Cur::element_type::otype;
    using types = meta::ReduceAllDefTypes<I, K, V, F>; 
    return ReduceAllBuilder<I, V, F, typename types::odefslct, K,
                            boost::hash<typename types::ktype>, Fl>{
        std::forward<F>(f), curUnit, _fl, false, 0};
  }
};
}
} // namespace ezl namespace ezl::detail

#endif // !DATAFLOWEXPR_EZL_H
