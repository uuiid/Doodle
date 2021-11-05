//
// Created by TD on 2021/7/27.
//
#include <doodle_lib/doodle_lib_all.h>

#include <catch.hpp>

TEST_CASE("test_gui", "[gui]") {
  using namespace doodle;
  FSys::path k_voide_file1{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0001.mp4)"};
  FSys::path k_voide_file2{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0002.mp4)"};
  FSys::path k_voide_file3{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0003.mp4)"};
  FSys::path k_voide_file4{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0004.mp4)"};

  auto& k_d    = doodle_lib::Get();

  auto k_app   = new_object<doodle_app>();

  auto k_reg   = g_reg();

  auto k_h     = make_handle(k_reg->create());
  k_h.emplace<opencv_read_player>().open_file(k_voide_file1);

  k_h = make_handle(k_reg->create());
  k_h.emplace<opencv_read_player>().open_file(k_voide_file2);

  k_h = make_handle(k_reg->create());
  k_h.emplace<opencv_read_player>().open_file(k_voide_file3);

  k_h = make_handle(k_reg->create());
  k_h.emplace<opencv_read_player>().open_file(k_voide_file4);
  k_app->run();
}