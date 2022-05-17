//
// Created by TD on 2021/7/26.
//

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#include <doodle_lib/app/app.h>
#include <doodle_core/core/doodle_lib.h>

#include <boost/locale.hpp>

int main(int argc, char *argv[]) {
  int k_r = Catch::Session().run(argc, argv);
  return k_r;
}
