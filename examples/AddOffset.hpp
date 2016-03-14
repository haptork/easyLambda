/*!
 * @file
 * class for calculating offset & closest lattice site of an atom
 *
 * Used with interstitialcount example.
 * */
#ifndef __ADDOFFSET_HPP__
#define __ADDOFFSET_HPP__

#include <array>
#include <string>
#include <tuple>
#include <vector>

class AddOffset {
public:
  AddOffset(float latConst, std::string lattice, std::array<float, 3> origin);
  std::tuple<std::array<float, 3>, float>
  operator()(const std::array<float, 3> &coords);

private:
  bool _isUnitcell(float x, float y, float z, float l,
                   std::array<float, 3> origin);
  void _bccUnitcell(std::vector<std::array<float, 3>> &v, float lc,
                    std::array<float, 3> origin);
  void _fccUnitcell(std::vector<std::array<float, 3>> &v, float lc,
                    std::array<float, 3> origin);
  float _calcDistMirror(std::array<float, 3> a, std::array<float, 3> b,
                        float size, std::array<bool, 3> &mirror);

  std::vector<std::array<float, 3>> _sites;
  float _latConst;
};

#endif //__ADDOFFSET_HPP__
