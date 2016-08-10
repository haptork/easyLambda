/*!
 * @file
 * Basic tests for `slct.hpp`
 *
 * This file is a part of easyLambda(ezl) project for parallel data
 * processing with modern C++ and MPI.
 *
 * @copyright Utkarsh Bhardwaj <haptork@gmail.com> 2015-2016
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying LICENSE.md or copy at * http://boost.org/LICENSE_1_0.txt)
 * */

#include <type_traits>

#include <ezl/helper/meta/slct.hpp>

namespace ezl {
namespace test {
using namespace ezl::detail::meta;

void slctTest() {

  // size
  static_assert(slct<1, 2, 3>{}.size  == 3, "");

  // filling slct
  static_assert(std::is_same<slct<1,2,3>, typename fillSlct<0, 3>::type>::value, "");
  static_assert(std::is_same<slct<3,4,5>, typename fillSlct<2, 5>::type>::value, "");
  static_assert(std::is_same<slct<3>, typename fillSlct<2, 3>::type>::value, "");
  static_assert(std::is_same<slct<>, typename fillSlct<2, 2>::type>::value, "");
  
  // Does not handle cases when L>N 
  //static_assert(std::is_same<slct<>, typename fillSlct<2, 0>::type>::value, "");


  // concatenate mask or slct
  static_assert(std::is_same<slct<3, 5, 6, 4>, 
                  typename ConcatenateSlct<slct<3, 5, 6>, slct<4>>::type>::value, ""
  );
  static_assert(std::is_same<slct<4>, 
                  typename ConcatenateSlct<slct<>, slct<4>>::type>::value, ""
  );

  static_assert(std::is_same<slct<3, 5, 6, 4>, 
                  typename ConcatenateManySlct<slct<3>, slct<5,6>, slct<4>>::type>::value, ""
  );
  static_assert(std::is_same<slct<4>, 
                  typename ConcatenateManySlct<slct<>, slct<4>>::type>::value, ""
  );

  // remove N from slct
  static_assert(std::is_same<slct<3,4>, RemoveNslct<2,3,4,2,2>>::value, "");
  static_assert(std::is_same<slct<3,4>, RemoveNslct<2,3,4>>::value, "");
  static_assert(std::is_same<slct<>, RemoveNslct<2,2,2>>::value, "");
  static_assert(std::is_same<slct<>, RemoveNslct<2>>::value, "");

  // Invert Slct of a particular length
  static_assert(std::is_same<slct<2,4,5>, InvertSlct<5, slct<1, 3>>>::value, "");
  static_assert(std::is_same<slct<2,3,4,5>, InvertSlct<5, slct<1, 6>>>::value, "");
  static_assert(std::is_same<slct<1, 2, 3>, InvertSlct<3, slct<>>>::value, "");
  static_assert(std::is_same<slct<2, 3>, InvertSlct<3, slct<1>>>::value, "");

  // this is specialized function to give the column manipulation slct to get as if the selected
  // input columns in a map UDF are replaced by its result.
  static_assert(std::is_same<slct<4, 5, 2, 3>, typename inPlace<3, 2, slct<1>>::type>::value, "");
  static_assert(std::is_same<slct<3>, typename inPlace<2, 1, slct<1,2>>::type>::value, "");
  static_assert(std::is_same<slct<>, typename inPlace<2, 0, slct<1,2>>::type>::value, "");
  static_assert(std::is_same<slct<>, typename inPlace<0, 0, slct<>>::type>::value, "");
  static_assert(std::is_same<slct<1,2>, typename inPlace<0, 2, slct<>>::type>::value, "");

  // checking if slct is only boolean and can be considered as mask
  static_assert(isBool<1,0,0,0,1,1>{}, "");
  static_assert(!isBool<1,2,3,4,5,5>{}, "");
  static_assert(!isBool<1>{}, "");
  static_assert(!isBool<>{}, "");
  static_assert(!isBool<1,0,2>{}, "");

  static_assert(std::is_same<slct<3,4>, saneSlct<4,0,0,1,1>>::value, "");
  static_assert(std::is_same<slct<1>, saneSlct<1,1>>::value, "");
  static_assert(std::is_same<slct<1,5,3>, saneSlct<5,1,5,3>>::value, "");
  static_assert(std::is_same<slct<1,5,3>, saneSlct<6,1,5,3>>::value, "");

  // displays nice static errors
  //static_assert(std::is_same<slct<1,5,3>, typename saneSlct<4,slct<1,5,3>>::type>::value, "");
  //static_assert(std::is_same<slct<1,0,3>, typename saneSlct<6,slct<1,0,3>>::type>::value, "");
  //static_assert(std::is_same<slct<3,4>, typename saneSlct<4,slct<0,0,1,1,1>>::type>::value, "");

}
}
}
