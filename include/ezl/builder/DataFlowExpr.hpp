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

#include <ezl/builder/FilterBuilder.hpp>
#include <ezl/builder/FilterExpr.hpp>
#include <ezl/builder/LoadUnitBuilder.hpp>
#include <ezl/builder/LoadUnitExpr.hpp>
#include <ezl/builder/MapBuilder.hpp>
#include <ezl/builder/MapExpr.hpp>
#include <ezl/builder/ReduceAllBuilder.hpp>
#include <ezl/builder/ReduceAllExpr.hpp>
#include <ezl/builder/ReduceBuilder.hpp>
#include <ezl/builder/ReduceExpr.hpp>
#include <ezl/builder/ParExpr.hpp>
#include <ezl/builder/DumpExpr.hpp>

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
template <class T> struct DataFlowExpr {

  DataFlowExpr() = default;

  template <class F> auto map(F&& f) {
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(curUnit)::element_type::otype;
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    using O = typename meta::MapDefTypesNoSlct<I, F>::odefslct;
    return MapBuilder<I, nomask, F, O, meta::slct<>, boost::hash<std::tuple<>>>{
      std::forward<F>(f), curUnit};
  }

  template <int N, int... Ns, class F> auto map(F&& f) {
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(curUnit)::element_type::otype;
    using S = meta::saneSlct<std::tuple_size<I>::value, N, Ns...>;
    using O = typename meta::MapDefTypes<I, S, F>::odefslct;
    return MapBuilder<I, S, F, O, meta::slct<>, boost::hash<std::tuple<>>>{
      std::forward<F>(f), curUnit};
  }

  template <int N, int... Ns, class F> auto filter(F&& f) {
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(curUnit)::element_type::otype;
    using S = meta::saneSlct<std::tuple_size<I>::value, N, Ns...>;
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    return FilterBuilder<I, S, F, nomask, meta::slct<>,
                         boost::hash<std::tuple<>>>{std::forward<F>(f),
                                                    curUnit};
  }

  template <class F> auto filter(F&& f) {
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(curUnit)::element_type::otype;
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    return FilterBuilder<I, nomask, F, nomask, meta::slct<>,
                         boost::hash<std::tuple<>>>{std::forward<F>(f),
                                                    curUnit};
  }

  template <int... Ns, class F, class FO, class... FOs>
  auto reduce(F&& f, FO&& fo, FOs&&... fos) {
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using K = meta::saneSlct<size, Ns...>;
    using V = meta::InvertSlct<size, K>;
    using types =
        meta::ReduceDefTypes<I, K, std::tuple<FO, FOs...>>;
    return ReduceBuilder<I, V, F, std::tuple<FO, FOs...>,
                         typename types::odefslct, K,
                         boost::hash<typename types::ktype>>{
        std::forward<F>(f),
        std::make_tuple(std::forward<FO>(fo), std::forward<FOs>(fos)...),
        std::move(curUnit), false};
  }

  template <int... Ns, class F, class FO>
  auto reduce(F&& f, FO&& fo) {
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using K = meta::saneSlct<size, Ns...>;
    using V = meta::InvertSlct<size, K>;
    using types = meta::ReduceDefTypes<I, K, std::tuple<FO>>;
    return ReduceBuilder<I, V, F, FO, typename types::odefslct, K,
                         boost::hash<typename types::ktype>>{
        std::forward<F>(f), std::forward<FO>(fo), curUnit, false};
  }

  template <int... Ns, class F> auto reduceAll(F&& f) {
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using K = meta::saneSlct<size, Ns...>;
    using V = meta::InvertSlct<size, K>;
    using types = meta::ReduceAllDefTypes<I, K, V, F>;
    return ReduceAllBuilder<I, V, F, typename types::odefslct,
                            K, boost::hash<typename types::ktype>>{
                              std::forward<F>(f), curUnit, false, 0};
  }

  template <class K, class V, class F, class FO, class... FOs>
  auto reduce(F&& f, FO&& fo, FOs&&... fos) {
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using saneK = typename meta::saneSlctImpl<size, K>::type;
    using saneV = typename meta::saneSlctImpl<size, V>::type;
    using types = meta::ReduceDefTypes<I, saneK,
                                            std::tuple<FO, FOs...>>;
    return ReduceBuilder<I, saneV, F, std::tuple<FO, FOs...>,
                         typename types::odefslct, saneK,
                         boost::hash<typename types::ktype>>{
        std::forward<F>(f),
        std::make_tuple(std::forward<FO>(fo), std::forward<FOs>(fos)...),
        std::move(curUnit), false};
  }

  template <class K, class V, class F, class FO>
  auto reduce(F&& f, FO&& fo) {
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using saneK = typename meta::saneSlctImpl<size, K>::type;
    using saneV = typename meta::saneSlctImpl<size, V>::type;
    using types = meta::ReduceDefTypes<I, saneK, std::tuple<FO>>;
    return ReduceBuilder<I, saneV, F, FO, typename types::odefslct, saneK,
                         boost::hash<typename types::ktype>>{
        std::forward<F>(f), std::forward<FO>(fo), curUnit, false};
  }

  template <class K, class V, class F> auto reduceAll(F&& f) {
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using saneK = typename meta::saneSlctImpl<size, K>::type;
    using saneV = typename meta::saneSlctImpl<size, V>::type;
    using types = meta::ReduceAllDefTypes<I, saneK, saneV, F>;
    return ReduceAllBuilder<I, saneV, F, typename types::odefslct, saneK,
                            boost::hash<typename types::ktype>>{
        std::forward<F>(f), curUnit, false, 0};
  }

  template <class Ptype> auto run(Ptype procs, bool refresh = true) {
    auto curUnit = ((T *)this)->build();
    if (curUnit) Karta::inst().run(curUnit.get(), ProcReq{procs});
    using I = typename decltype(curUnit)::element_type;
    LoadUnitBuilder<I> obj;
    return obj.prev(curUnit);
  }

  auto run(std::initializer_list<int> lprocs = {}, bool refresh = true) {
    std::vector<int> procs(std::begin(lprocs), std::end(lprocs));
    auto curUnit = ((T *)this)->build();
    if (procs.empty()) {
      if (curUnit) Karta::inst().run(curUnit.get(), ProcReq{});
    } else {
      if (curUnit) Karta::inst().run(curUnit.get(), ProcReq{procs});
    }
    using I = typename decltype(curUnit)::element_type;
    LoadUnitBuilder<I> obj;
    return obj.prev(curUnit);
  }

  template <class Ptype> auto runResult(Ptype procs, bool refresh = true) {
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(curUnit)::element_type::otype;
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    std::vector<typename meta::SlctTupleType<I>::type> buffer;
    if (!curUnit) return buffer;
    auto dumper = dumpMem(buffer);
    auto f = std::make_shared<Filter<I, nomask, decltype(dumper), nomask>>(dumper);
    curUnit->next(f, curUnit);
    if (refresh) Karta::inst().refresh();
    Karta::inst().run(f.get(), ProcReq{procs});
    return buffer;
  }

  auto runResult(std::initializer_list<int> lprocs = {}, bool refresh = true) {
    std::vector<int> procs(std::begin(lprocs), std::end(lprocs));
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(curUnit)::element_type::otype;
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    std::vector<typename meta::SlctTupleType<I>::type> buffer;
    if (!curUnit) return buffer;
    auto dumper = dumpMem(buffer);
    auto f = std::make_shared<Filter<I, nomask, decltype(dumper), nomask>>(dumper);
    curUnit->next(f, curUnit);
    if (refresh) Karta::inst().refresh();
    if (procs.empty()) {
      Karta::inst().run(f.get(), ProcReq{});
    } else {
      Karta::inst().run(f.get(), ProcReq{procs});
    }
    return buffer;
  }

  auto oneUp() {
    auto pre = ((T *)this)->prev();
    auto curUnit = ((T *)this)->build();
    using I = typename decltype(pre)::element_type;
    LoadUnitBuilder<I> obj;
    return obj.prev(pre);
  }

  template <class I>
  auto branchFlow(I nx) {
    auto curUnit = ((T *)this)->build();
    if (curUnit) curUnit->next(nx, curUnit);
    LoadUnitBuilder<typename decltype(curUnit)::element_type> obj;
    return obj.prev(curUnit);
  }

  template <class I>
  auto addFlow(I nx) {
    auto curUnit = ((T *)this)->build();
    if (curUnit) curUnit->next(nx, curUnit);
    LoadUnitBuilder<typename I::element_type> obj;
    return obj.prev(nx);
  }

};
}
} // namespace ezl namespace ezl::detail

#endif // !DATAFLOWEXPR_EZL_H
