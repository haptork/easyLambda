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
protected:
  DataFlowExpr() = default;
public:
  template <class F> auto map(F&& f) {
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    using O = typename meta::MapDefTypesNoSlct<I, F>::odefslct;
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    _addCurToFlow(curUnit, _fl);
    auto nfl = std::make_shared<Nfl>(_fl);
    return MapBuilder<I, nomask, F, O, meta::slct<>, boost::hash<std::tuple<>>, Nfl>{
      std::forward<F>(f), curUnit, nfl};
  }

  template <int N, int... Ns, class F> auto map(F&& f) {
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using S = meta::saneSlct<size, N, Ns...>;
    using O = typename meta::MapDefTypes<I, S, F>::odefslct;
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    _addCurToFlow(curUnit, _fl);
    auto nfl = std::make_shared<Nfl>(_fl);
    return MapBuilder<I, S, F, O, meta::slct<>, boost::hash<std::tuple<>>, Nfl>{
      std::forward<F>(f), curUnit, nfl};
  }

  template <int N, int... Ns, class F> auto filter(F&& f) {
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    using S = meta::saneSlct<std::tuple_size<I>::value, N, Ns...>;
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    _addCurToFlow(curUnit, _fl);
    auto nfl = std::make_shared<Nfl>(_fl);
    return FilterBuilder<I, S, F, nomask, meta::slct<>,
                         boost::hash<std::tuple<>>, Nfl>{std::forward<F>(f),
                                                         curUnit, nfl};
  }

  template <class F> auto filter(F&& f) {
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    _addCurToFlow(curUnit, _fl);
    auto nfl = std::make_shared<Nfl>(_fl);
    return FilterBuilder<I, nomask, F, nomask, meta::slct<>,
                         boost::hash<std::tuple<>>, Nfl>{std::forward<F>(f),
                                                         curUnit, nfl};
  }

  template <class K1, class K2, class Pre2> auto zip(Pre2 pre2) {
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    using I2 = typename decltype(pre2)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    constexpr auto size2 = std::tuple_size<I2>::value;
    using saneK1 = typename meta::saneSlctImpl<size, K1>::type;
    using saneK2 = typename meta::saneSlctImpl<size2, K2>::type;
    using ktype = typename meta::SlctTupleRefType<I, saneK1>::type;
    using nomask = typename meta::fillSlct<0, size + size2>::type;
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    _addCurToFlow(curUnit, _fl);
    auto nfl = std::make_shared<Nfl>(_fl);
    return ZipBuilder<I, I2, saneK1, saneK2, nomask,
                      boost::hash<std::tuple<ktype>>, Nfl>{curUnit, pre2, nfl};
  }

  template <int... Ks, class Pre2> auto zip(Pre2 pre2) {
    return zip<meta::slct<Ks...>, meta::slct<Ks...>, Pre2>(pre2);
  }

  template <int... Ns, class F, class FO, class... FOs>
  auto reduce(F&& f, FO&& fo, FOs&&... fos) {
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using K = meta::saneSlct<size, Ns...>;
    using V = meta::InvertSlct<size, K>;
    using types =
        meta::ReduceDefTypes<I, K, std::tuple<FO, FOs...>>;
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    _addCurToFlow(curUnit, _fl);
    auto nfl = std::make_shared<Nfl>(_fl);
    return ReduceBuilder<I, V, F, std::tuple<FO, FOs...>,
                         typename types::odefslct, K,
                         boost::hash<typename types::ktype>, Nfl>{
        std::forward<F>(f),
        std::make_tuple(std::forward<FO>(fo), std::forward<FOs>(fos)...),
        std::move(curUnit), nfl, false};
  }

  template <int... Ns, class F, class FO>
  auto reduce(F&& f, FO&& fo) {
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using K = meta::saneSlct<size, Ns...>;
    using V = meta::InvertSlct<size, K>;
    using types = meta::ReduceDefTypes<I, K, std::tuple<FO>>;
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    _addCurToFlow(curUnit, _fl);
    auto nfl = std::make_shared<Nfl>(_fl);
    return ReduceBuilder<I, V, F, FO, typename types::odefslct, K,
                         boost::hash<typename types::ktype>, Nfl>{
        std::forward<F>(f), std::forward<FO>(fo), curUnit, nfl, false};
  }

  template <int... Ns, class F> auto reduceAll(F&& f) {
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using K = meta::saneSlct<size, Ns...>;
    using V = meta::InvertSlct<size, K>;
    using types = meta::ReduceAllDefTypes<I, K, V, F>;
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    _addCurToFlow(curUnit, _fl);
    auto nfl = std::make_shared<Nfl>(_fl);
    return ReduceAllBuilder<I, V, F, typename types::odefslct,
                            K, boost::hash<typename types::ktype>, Nfl>{
                              std::forward<F>(f), curUnit, nfl, false, 0};
  }

  template <class K, class V, class F, class FO, class... FOs>
  auto reduce(F&& f, FO&& fo, FOs&&... fos) {
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using saneK = typename meta::saneSlctImpl<size, K>::type;
    using saneV = typename meta::saneSlctImpl<size, V>::type;
    using types = meta::ReduceDefTypes<I, saneK,
                                            std::tuple<FO, FOs...>>;
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    _addCurToFlow(curUnit, _fl);
    auto nfl = std::make_shared<Nfl>(_fl);
    return ReduceBuilder<I, saneV, F, std::tuple<FO, FOs...>,
                         typename types::odefslct, saneK,
                         boost::hash<typename types::ktype>, Nfl>{
        std::forward<F>(f),
        std::make_tuple(std::forward<FO>(fo), std::forward<FOs>(fos)...),
        std::move(curUnit), nfl, false};
  }

  template <class K, class V, class F, class FO>
  auto reduce(F&& f, FO&& fo) {
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using saneK = typename meta::saneSlctImpl<size, K>::type;
    using saneV = typename meta::saneSlctImpl<size, V>::type;
    using types = meta::ReduceDefTypes<I, saneK, std::tuple<FO>>;
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    _addCurToFlow(curUnit, _fl);
    auto nfl = std::make_shared<Nfl>(_fl);
    return ReduceBuilder<I, saneV, F, FO, typename types::odefslct, saneK,
                         boost::hash<typename types::ktype>, Nfl>{
        std::forward<F>(f), std::forward<FO>(fo), curUnit, nfl, false};
  }

  template <class K, class V, class F> auto reduceAll(F&& f) {
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    constexpr auto size = std::tuple_size<I>::value;
    using saneK = typename meta::saneSlctImpl<size, K>::type;
    using saneV = typename meta::saneSlctImpl<size, V>::type;
    using types = meta::ReduceAllDefTypes<I, saneK, saneV, F>;
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    _addCurToFlow(curUnit, _fl);
    auto nfl = std::make_shared<Nfl>(_fl);
    return ReduceAllBuilder<I, saneV, F, typename types::odefslct, saneK,
                            boost::hash<typename types::ktype>, Nfl>{
        std::forward<F>(f), curUnit, nfl, false, 0};
  }

  auto build() {
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(curUnit)::element_type::otype;
    _addCurToFlow(curUnit, _fl);
    auto fl = std::make_shared<Flow<typename Fl::itype, I>>(_fl); 
    fl->addLast(curUnit);
    return fl;
  }

  template <class Ptype> auto run(Ptype procs, bool refresh = true) {
    auto fl = build();
    if (refresh) Karta::inst().refresh();
    if (fl) Karta::inst().run(fl.get(), ProcReq{procs});
    return fl;
  }

  auto run(std::initializer_list<int> lprocs = {}, bool refresh = true) {
    std::vector<int> procs(std::begin(lprocs), std::end(lprocs));
    return run(procs, refresh);
  }

  template <class Ptype> auto runResult(Ptype procs, bool refresh = true) {
    auto fl = build();
    using I = typename decltype(fl)::element_type::otype;
    std::vector<typename meta::SlctTupleType<I>::type> buffer;
    if (fl->isEmpty()) return buffer;
    auto dumper = dumpMem(buffer);
    using nomask =
        typename meta::fillSlct<0, std::tuple_size<I>::value>::type;
    auto filter =
        std::make_shared<Filter<I, nomask, decltype(dumper), nomask>>(dumper);
    fl->next(filter, fl);
    if (refresh) Karta::inst().refresh();
    Karta::inst().run(filter.get(), ProcReq{procs});
    fl->unNext(filter.get());
    return buffer;
  }

  auto runResult(std::initializer_list<int> lprocs = {}, bool refresh = true) {
    std::vector<int> procs(std::begin(lprocs), std::end(lprocs));
    return runResult(procs, refresh);
  }

  auto oneUp() {
    auto pre = ((T *)this)->prev();
    auto curUnit = ((T *)this)->buildUnit();
    using I = typename decltype(pre)::element_type;
    _addCurToFlow(curUnit, _fl);
    return LoadUnitBuilder<I, decltype(((T*)this)->isAddFirst), Fl> {pre, _fl};
  }

  template <class I>
  auto branchFlow(I nx) {
    auto curUnit = ((T *)this)->buildUnit();
    nx->prev(curUnit, nx);
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    _addCurToFlow(curUnit, _fl);
    auto nfl = std::make_shared<Nfl>(_fl);
    return LoadUnitBuilder<typename decltype(curUnit)::element_type,
                           decltype(((T*)this)->isAddFirst), Nfl>{curUnit, nfl};
  }

  template <class I>
  auto addFlow(I nx) {
    auto curUnit = ((T *)this)->buildUnit();
    nx->prev(curUnit, nx);
    using Nfl = Flow<typename Fl::itype, decltype(((T*)this)->isAddFirst)>;
    auto nfl = std::make_shared<Nfl>(_fl);
    _addCurToFlow(curUnit, nfl);
    return LoadUnitBuilder<typename I::element_type, std::false_type, Nfl>{nx,
                                                                           nfl};
  }

protected:
  std::shared_ptr<Fl> _fl;

private:
  template <class I>
  auto _addCurToFlow(
      std::shared_ptr<I> curUnit,
      std::shared_ptr<Flow<typename Fl::itype, std::true_type>> fl) {
    fl->addFirst(curUnit);
  }

  template <class I>
  auto _addCurToFlow(
      std::shared_ptr<I> curUnit,
      std::shared_ptr<Flow<typename Fl::itype, std::false_type>> fl) {}
};
}
} // namespace ezl namespace ezl::detail

#endif // !DATAFLOWEXPR_EZL_H
