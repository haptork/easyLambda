/*!
 * @file
 * glob wrapper function returning vector of strings
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */
#ifndef VGLOB_EZL_H
#define VGLOB_EZL_H

#include <glob.h>
#include <string>
#include <vector>

namespace ezl {
namespace detail {

// glob that returns vector of file name strings.
inline auto vglob(const std::string &pat, size_t max = 0) {
  glob_t glob_result;
  glob(pat.c_str(), GLOB_TILDE, NULL, &glob_result);
  std::vector<std::string> ret;
  for (unsigned int i = 0; i < glob_result.gl_pathc; ++i) {
    ret.push_back(std::string(glob_result.gl_pathv[i]));
    if (max && i>=max) break;
  }
  globfree(&glob_result);
  return ret;
}

} // naepsace detail
} // namespace ezl

#endif // ! VGLOB_EZL_H
