//
// Created by TD on 2021/7/26.
//


#include <doodle_lib/doodle_lib_all.h>
#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

int main(int argc, char *argv[]) {
  //初始化测试环境
  std::setlocale(LC_CTYPE, ".UTF8");
  auto k_doodle = doodle::new_object<doodle::doodle_lib>();

  int k_r = Catch::Session().run(argc, argv);
  return k_r;
}
