//
// Created by TD on 2022/5/17.
//
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/app/app.h>
using namespace doodle;
TEST_CASE("test json rpc") {
  auto l_app = app{};

  l_app.run();
}
