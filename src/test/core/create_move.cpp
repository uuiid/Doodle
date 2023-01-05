//
// Created by TD on 2022/10/8.
//

#include <doodle_core/doodle_core.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>

#include <doodle_lib/app/doodle_main_app.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <boost/test/unit_test.hpp>

#include <crtdbg.h>
#include <main_fixtures/lib_fixtures.h>
#include <stdlib.h>


namespace doodle::facet {
class teste_facet : public gui_facet {
 public:
 protected:
  void load_windows() override {}
};
}  // namespace doodle::facet

namespace doodle {
class test_app : public doodle::doodle_main_app {
 public:
  test_app() : doodle::doodle_main_app() {
    run_facet = std::make_shared<facet::teste_facet>();
    add_facet(run_facet);
  }
};
}  // namespace doodle

using namespace doodle;

struct move_fix {
  move_fix() { timer = std::make_shared<boost::asio::high_resolution_timer>(g_io_context()); }
  void setup() {
    for (int l = 0; l < 500; ++l) {
      main_app_attr.poll_one();
    }
  }

  test_app main_app_attr;
  std::shared_ptr<boost::asio::high_resolution_timer> timer;
};

BOOST_FIXTURE_TEST_SUITE(move, move_fix, *boost::unit_test::disabled())

BOOST_AUTO_TEST_CASE(base_run) {
  bool run_test{};
  timer->async_wait([l_r = &run_test](boost::system::error_code) {
    app_base::Get().stop_app();
    *l_r = true;
  });
  timer->expires_from_now(2s);
  main_app_attr.run();
  BOOST_TEST(run_test);
}

BOOST_AUTO_TEST_CASE(create) {
  auto l_h = make_handle();
  FSys::path l_image_path{R"(E:\tmp\image_test_ep002_sc001)"};
  l_h.emplace<episodes>().analysis(l_image_path);
  l_h.emplace<shot>().analysis(l_image_path);
  l_h.emplace<FSys::path>(l_image_path.parent_path());

  std::vector<FSys::path> l_files{FSys::list_files(l_image_path)};
  bool run_test{};
  g_reg()->ctx().at<image_to_move>()->async_create_move(l_h, l_files, [l_r = &run_test, this]() {
    *l_r = true;
    timer->async_wait([](boost::system::error_code) { app_base::Get().stop_app(); });
    timer->expires_from_now(2s);
  });

  main_app_attr.run();
  BOOST_TEST(run_test);
}

BOOST_AUTO_TEST_SUITE_END()
