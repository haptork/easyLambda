/*!
 * @file
 * src for AddOffset class.
 *
 * Used with interstitialcount example.
 * */
#include "AddOffset.hpp"

bool AddOffset::_isUnitcell(float x, float y, float z, float l,
                            std::array<float, 3> origin) {
  float pos[3] = {x, y, z};
  for (int i = 0; i < 3; i++) {
    if (!(pos[i] >= origin[i] * l and pos[i] <= origin[i] * l + l))
      return false;
  }
  return true;
}

void AddOffset::_bccUnitcell(std::vector<std::array<float, 3>> &v, float lc,
                             std::array<float, 3> origin) {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        if (_isUnitcell(i * lc, j * lc, k * lc, lc, origin)) {
          std::array<float, 3> p1;
          p1[0] = (i - origin[0]) * lc, p1[1] = (j - origin[1]) * lc,
          p1[2] = (k - origin[2]) * lc;
          v.push_back(std::move(p1));
        }
        if (v.size() == 2) return;
        if (_isUnitcell((i + 0.5) * lc, (j + 0.5) * lc, (k + 0.5) * lc, lc,
                        origin)) {
          std::array<float, 3> p1;
          p1[0] = (i + 0.5 - origin[0]) * lc,
          p1[1] = (j + 0.5 - origin[1]) * lc,
          p1[2] = (k + 0.5 - origin[2]) * lc;
          v.push_back(std::move(p1));
        }
        if (v.size() == 2) return;
      }
    }
  }
}

void AddOffset::_fccUnitcell(std::vector<std::array<float, 3>> &v, float lc,
                             std::array<float, 3> origin) {
  for (int i = -1; i < 3; i++) {
    for (int j = -1; j < 3; j++) {
      for (int k = -1; k < 3; k++) {
        if (_isUnitcell(i * lc, j * lc, k * lc, lc, origin)) {
          std::array<float, 3> p1;
          p1[0] = (i - origin[0]) * lc, p1[1] = (j - origin[1]) * lc,
          p1[2] = (k - origin[2]) * lc;
          v.push_back(std::move(p1));
        }
        if (v.size() == 4) return;
        if (_isUnitcell(i * lc, (j + 0.5) * lc, (k + 0.5) * lc, lc, origin)) {
          std::array<float, 3> p1;
          p1[0] = (i - origin[0]) * lc, p1[1] = (j + 0.5 - origin[1]) * lc,
          p1[2] = (k + 0.5 - origin[2]) * lc;
          v.push_back(std::move(p1));
        }
        if (v.size() == 4) return;
        if (_isUnitcell((i + 0.5) * lc, j * lc, (k + 0.5) * lc, lc, origin)) {
          std::array<float, 3> p1;
          p1[0] = (i + 0.5 - origin[0]) * lc, p1[1] = (j - origin[1]) * lc,
          p1[2] = (k + 0.5 - origin[2]) * lc;
          v.push_back(std::move(p1));
        }
        if (v.size() == 4) return;
        if (_isUnitcell((i + 0.5) * lc, (j + 0.5) * lc, k * lc, lc, origin)) {
          std::array<float, 3> p1;
          p1[0] = (i + 0.5 - origin[0]) * lc,
          p1[1] = (j + 0.5 - origin[1]) * lc, p1[2] = (k - origin[2]) * lc;
          v.push_back(std::move(p1));
        }
        if (v.size() == 4) return;
      }
    }
  }
}

float AddOffset::_calcDistMirror(std::array<float, 3> a, std::array<float, 3> b,
                                 float size, std::array<bool, 3> &mirror) {
  float res = 0;
  for (int i = 0; i < 3; i++) {
    float dist = fabsf(a[i] - b[i]);
    if (dist > size * 0.5) {
      dist = size - dist;
      mirror[i] = 1;
    } else {
      mirror[i] = 0;
    }
    res += dist * dist;
  }
  return std::sqrt(res);
}

AddOffset::AddOffset(float latConst, std::string lattice,
                     std::array<float, 3> origin) {
  _latConst = latConst;
  if (lattice[0] == 'b') {
    _bccUnitcell(_sites, _latConst, origin);
  } else if (lattice[0] == 'f') {
    _fccUnitcell(_sites, _latConst, origin);
  } else {
    // TODO log error
  }
}

std::tuple<std::array<float,3>, float>
AddOffset::operator()(const std::array<float, 3> &coords) {
  std::array<float, 3> modCoords;
  std::array<float, 3> divCoords;
  std::array<float, 3> cellPos;

  for (int i = 0; i < 3; i++) {
    modCoords[i] = fmodf(coords[i], _latConst);
    divCoords[i] = int(coords[i] / _latConst);
  }
  auto min = -1.F;
  auto i = 0;
  std::array<bool, 3> mirror;
  for (auto it : _sites) {
    auto temp = _calcDistMirror(it, modCoords, _latConst, mirror);
    if (temp < min || min == -1) {
      min = temp;
      for (int i = 0; i < 3; i++) {
        cellPos[i] = (divCoords[i] + mirror[i] + it[i] / _latConst);
        cellPos[i] = floorf(cellPos[i] * 100) / 100;
      }
    }
    i++;
  }
  return std::make_tuple(cellPos, min);
}
