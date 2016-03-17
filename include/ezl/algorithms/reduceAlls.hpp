/*!
 * @file
 * Function objects implementing basic algos for reduceAll
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#ifndef REDUCEALLS_EZL_ALGO_H
#define REDUCEALLS_EZL_ALGO_H

#include <tuple>
#include <vector>
#include <algorithm>
#include <numeric>

namespace ezl {

/*!
 * @ingroup algorithms
 * function object for calculating hist frequency bins of various columns.
 * works fine with std::array as columns.
 *
 * */
class hist {
public:
  hist(int nBin, double min = 0.0, double max = 0.0)
      : _nBin{nBin}, _min{min}, _max{max} {}

  hist(double binSize, double min = 0.0, double max = 0.0)
      : _binSize{binSize}, _min{min}, _max{max} {}

  template <class... Ks, class... Vs>
  auto operator()(const std::tuple<Ks...> &, const std::tuple<Vs...> &val) {
    return _hist(val, std::make_index_sequence<sizeof...(Vs)>{});
  };

private:
  auto _findRange(double &min, double &max, int &nBin, double &binSize) {
    if (nBin == 0) {
      if (min != _min || max != _max) {
        min -= binSize / 2;
        max += binSize / 2;
      }
      auto diff = max - min;
      nBin = (int)ceil(diff / binSize);
    } else if (binSize == 0.0) {
      auto diff = max - min;
      binSize = diff / nBin;
    }
  }

  template <class V>
  auto _vectorHist(V &vals, int nBin, double min, double binSize) {
    std::vector<int> res(nBin, 0);
    for (const auto it : vals) {
      auto bin = floor(((it - min) / binSize));
      if (bin >= nBin) bin = nBin - 1;
      if (bin < 0) bin = 0;
      res[bin] += 1;
    }
    return res;
  }

  template <typename Tup, std::size_t... index>
  inline auto _hist(const Tup &tup, std::index_sequence<index...>) {
    auto min = _min;
    auto max = _max;
    auto nBin = _nBin;
    auto binSize = _binSize;
    if (min == max) {
      auto minmax =
          std::minmax_element(std::get<0>(tup).begin(), std::get<0>(tup).end());
      min = *minmax.first;
      max = *minmax.second;
    }
    _findRange(min, max, nBin, binSize);
    std::array<std::vector<int>, sizeof...(index)> hists{
        {_vectorHist(std::get<index>(tup), nBin, min, binSize)...}};
    std::vector<std::tuple<std::array<double, 2>,
                           std::array<int, sizeof...(index)>>> res(nBin);
    for (auto i = 0; i < nBin; i++) {
      std::get<0>(res[i]) =
          std::array<double, 2>{{min + binSize * i, min + binSize * (i + 1)}};
      for (int j = 0; j < int(sizeof...(index)); j++) {
        std::get<1>(res[i])[j] = hists[j][i];
      }
    }
    return res;
  }

  int _nBin{0};
  double _binSize{0.0};
  double _min;
  double _max;
};

/*!
 * @ingroup algorithms
 * callable for calculating summary of various columns.
 * works fine with std::array as columns.
 *
 * */
class summary {
public:
  template <class... Ks, class... Vs>
  auto operator()(const std::tuple<Ks...> &, const std::tuple<Vs...> &val) {
    std::vector<std::array<double, 5>> res;
    res.reserve(sizeof...(Vs));
    _summary(res, val, std::make_index_sequence<sizeof...(Vs)>{});
    //std::reverse(std::begin(res), std::end(res));
    return res;
  }

private:
  template <class V, size_t N>
  bool _vectorSummary(const std::vector<std::array<V, N>> &vals,
                      std::vector<std::array<double, 5>> &res) {
    auto count = vals.size();
    if (count == 0) return 0;
    for (auto j = int(N - 1); j >= 0; j--) {
      std::array<double, 5> cur;
      double sum = 0.0;
      cur[3] = double(vals[0][j]);
      cur[2] = double(vals[0][j]);
      for (const auto &it : vals) {
        sum += it[j];
        if (it[j] > cur[3]) cur[3] = it[j];
        if (it[j] < cur[2]) cur[2] = it[j];
      }
      cur[0] = sum / count;
      std::vector<double> diff(count);
      for (size_t i = 0; i < count; i++) {
        diff[i] = vals[i][j] - cur[0];
      }
      double sq_sum =
          std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
      cur[1] = std::sqrt(sq_sum / double(count));
      cur[4] = double(count);
      res.push_back(cur);
    }
    return 0;
  }

  template <class V>
  bool _vectorSummary(const std::vector<V> &vals,
                      std::vector<std::array<double, 5>> &res) {
    auto count = vals.size();
    double sum = std::accumulate(vals.begin(), vals.end(), 0.0);
    double mean = sum / count;
    std::vector<double> diff(count);
    std::transform(vals.begin(), vals.end(), diff.begin(),
                   std::bind2nd(std::minus<double>(), mean));
    double sq_sum =
        std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / double(count));
    auto minmax = std::minmax_element(vals.begin(), vals.end());
    res.push_back(
        std::array<double, 5>{{mean, stdev, double(*(minmax.first)),
                               double(*(minmax.second)), double(count)}});
    return 0;
  }

  template <typename Tup, std::size_t... index>
  inline auto _summary(std::vector<std::array<double, 5>> &res, const Tup &tup,
                       std::index_sequence<index...>) {
    std::make_tuple(_vectorSummary(std::get<index>(tup), res)...);
  }
};

/*!
 * @ingroup algorithms
 * function object for calculating correlation of various columns.
 * works fine with std::array as columns.
 *
 * */
class corr {
public:
  // template<class... Vs>
  template <class... Ks, class... Vs>
  auto operator()(const std::tuple<Ks...> &, const std::tuple<Vs...> &val) {
    std::vector<double> res;
    res.reserve(sizeof...(Vs));
    std::vector<double> diff(get<0>(val).size());
    double sqsum = _calcRefDiff(get<0>(val), diff);
    _corr(val, diff, sqsum, std::make_index_sequence<sizeof...(Vs)>{}, res);
    //std::reverse(std::begin(res), std::end(res));
    return std::make_tuple(res);
  }

private:
  template <class V>
  auto _calcRefDiff(const V &val, std::vector<double> &diff) {
    return _calcDiff(val, diff);
  }

  template <class V, size_t N>
  auto _calcRefDiff(const std::vector<std::array<V, N>> &val,
                 std::vector<double> &diff) {
    return _calcDiffAr(val, diff, 0);
  }

  template <class V>
  auto _calcDiff(const V &val, std::vector<double> &diff) {
    double sum = 0.0;
    for (const auto &it : val) {
      sum += it;
    }
    double mean = sum / val.size();
    for (size_t i = 0; i < val.size(); i++) {
      diff[i] = val[i] - mean;
    }
    double sqsum = 0.0;
    for (const auto &it : diff) sqsum += (it * it);
    return sqsum;
  }

  template <class V>
  auto _calcDiffAr(const V &val, std::vector<double> &diff, int index) {
    double sum = 0.0;
    for (const auto &it : val) {
      sum += it[index];
    }
    double mean = sum / val.size();
    {auto i = 0;
    for (const auto &it :val) {
      diff[i++] = it[index] - mean;
    }}
    auto sqsum = 0.0;
    for (const auto &it : diff) sqsum += (it * it);
    return sqsum;
  }

  auto _crossSq(const std::vector<double> &diff1,
                const std::vector<double> &diff2) {
    auto crossSqsum = 0.0;
    for (size_t i = 0; i < diff1.size(); i++) {
      crossSqsum += (diff1[i] * diff2[i]);
    } 
    return crossSqsum;
  }

  template <class V>
  auto _vectorCorr(const V &val, const std::vector<double> &rDiff,
                   const double &rSqsum, std::vector<double> &diff,
                   std::vector<double> &res) {
    auto sqsum = _calcDiff(val, diff);
    res.push_back(_crossSq(diff, rDiff) / sqrt(sqsum * rSqsum));
    return false;
  }

  template <class V, size_t N>
  auto _vectorCorr(const std::vector<std::array<V, N>> &val,
                   const std::vector<double> &rDiff, const double &rSqsum,
                   std::vector<double> &diff, std::vector<double> &res) {
    for (size_t i = 0; i < N; i++) {
      auto sqsum = _calcDiffAr(val, diff, i);
      res.push_back(_crossSq(diff, rDiff) / sqrt(sqsum * rSqsum));
    }
    return false;
  }

  template <typename T, std::size_t... index>
  inline auto _corr(const T &val, const std::vector<double> &rDiff,
                    const double &rSqsum, std::index_sequence<index...>,
                    std::vector<double> &res) {
    std::vector<double> diff(std::get<0>(val).size());
    std::make_tuple(
        _vectorCorr(std::get<index>(val), rDiff, rSqsum, diff, res)...);
  }
  
};

} // namespace ezl

#endif // !REDUCES_EZL_ALGO_H
