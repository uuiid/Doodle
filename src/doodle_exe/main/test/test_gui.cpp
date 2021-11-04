//
// Created by TD on 2021/7/27.
//
#include <doodle_lib/doodle_lib_all.h>

#include <catch.hpp>

TEST_CASE("test_gui", "[gui]") {
  using namespace doodle;
  FSys::path k_voide_file{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0000.mp4)"};

  auto& k_d   = doodle_lib::Get();

  auto k_app  = new_object<doodle_app>();

  auto k_reg  = g_reg();
  auto k_h    = make_handle(k_reg->create());
  auto &k_open = k_h.emplace<opencv_read_player>();
  k_open.open_file(k_voide_file);
  k_app->run();
}