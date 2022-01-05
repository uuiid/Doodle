//
// Created by TD on 2021/7/27.
//
#include <doodle_lib/doodle_lib_all.h>

#include <catch.hpp>

TEST_CASE("for", "[cpp]") {
  for (int l_i = 10; l_i >= 0; --l_i) {
    std::cout << l_i << std::endl;
  }
}
