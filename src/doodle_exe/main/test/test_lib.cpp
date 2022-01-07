//
// Created by TD on 2022/1/7.
//

#include <doodle_lib/doodle_lib_all.h>

#include <catch.hpp>

using namespace doodle;
TEST_CASE("json") {
  nlohmann::json k_j{};
  k_j[0] = "sdsads";
  std::cout << k_j.dump() << std::endl;
}
