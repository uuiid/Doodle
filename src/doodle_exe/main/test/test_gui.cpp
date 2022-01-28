//
// Created by TD on 2021/7/27.
//
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/app/app.h>
#include <doodle_lib/gui/get_input_dialog.h>
#include <doodle_lib/gui/widgets/screenshot_widget.h>

#include <catch.hpp>

using namespace doodle;
// TEST_CASE("test_gui", "[gui]") {
//   FSys::path k_voide_file1{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0001.mp4)"};
//   FSys::path k_voide_file2{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0002.mp4)"};
//   FSys::path k_voide_file3{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0003.mp4)"};
//   FSys::path k_voide_file4{R"(D:\Doodle\data\case_tset\ep0000\sc0000\tset_0\None_\user_ep0000_sc0004.mp4)"};
//
//   auto& k_d  = doodle_lib::Get();
//
//   auto k_app = std::make_shared<app>();
//
//   auto k_reg = g_reg();
//
//   auto k_h   = make_handle(k_reg->create());
//   k_h.emplace<opencv_read_player>().open_file(k_voide_file1);
//
//   k_h = make_handle(k_reg->create());
//   k_h.emplace<opencv_read_player>().open_file(k_voide_file2);
//
//   k_h = make_handle(k_reg->create());
//   k_h.emplace<opencv_read_player>().open_file(k_voide_file3);
//
//   k_h = make_handle(k_reg->create());
//   k_h.emplace<opencv_read_player>().open_file(k_voide_file4);
//   k_app->run();
// }

class test_app : public app {
 public:
 protected:
  void load_windows() override {
    auto k_h = make_handle();
    k_h.emplace<project>();
    g_main_loop()
        .attach<null_process_t>()
        .then<get_input_project_dialog>(k_h)
        .then<one_process_t>([]() {
          app::Get().stop();
        });
  }
  void load_back_end() override {
  }
};

TEST_CASE_METHOD(test_app, "test get input") {
  app::Get().run();
}

class test_screenshot_widget_app : public app {
 public:
 protected:
  void load_windows() override {
    g_reg()->set<project>("D:/tmp", "test");
    auto k_h = make_handle();
    g_main_loop()
        .attach<null_process_t>()
        .then<screenshot_widget>(k_h)
        .then<one_process_t>([]() {
          app::Get().stop();
        });
  }
  void load_back_end() override {
  }
};

TEST_CASE_METHOD(test_screenshot_widget_app, "test_screenshot_widget") {
  app::Get().run();
}
