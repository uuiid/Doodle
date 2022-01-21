//
// Created by TD on 2021/7/27.
//
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/app/app.h>
#include <doodle_lib/gui/get_input_dialog.h>
#include <catch.hpp>

using namespace doodle;
TEST_CASE("test_gui", "[gui]") {
  FSys::path k_voide_file1{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0001.mp4)"};
  FSys::path k_voide_file2{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0002.mp4)"};
  FSys::path k_voide_file3{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0003.mp4)"};
  FSys::path k_voide_file4{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0004.mp4)"};

  auto& k_d  = doodle_lib::Get();

  auto k_app = std::make_shared<app>();

  auto k_reg = g_reg();

  auto k_h   = make_handle(k_reg->create());
  k_h.emplace<opencv_read_player>().open_file(k_voide_file1);

  k_h = make_handle(k_reg->create());
  k_h.emplace<opencv_read_player>().open_file(k_voide_file2);

  k_h = make_handle(k_reg->create());
  k_h.emplace<opencv_read_player>().open_file(k_voide_file3);

  k_h = make_handle(k_reg->create());
  k_h.emplace<opencv_read_player>().open_file(k_voide_file4);
  k_app->run();
}

TEST_CASE("test get input") {
  app l_app{};
  auto k_h = make_handle();
  k_h.emplace<project>();
  g_main_loop().attach<get_input_project_dialog>(k_h);
  l_app.run();
}
