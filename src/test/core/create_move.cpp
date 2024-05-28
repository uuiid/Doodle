//
// Created by TD on 2022/10/8.
//

#include <doodle_core/core/program_info.h>
#include <doodle_core/doodle_core.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>

#include <doodle_lib/facet/main_facet.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <boost/test/unit_test.hpp>

#include <crtdbg.h>
#include <stdlib.h>

using namespace doodle;

namespace {
struct loop_ {
  doodle_lib lib{};
};
}  // namespace
BOOST_FIXTURE_TEST_SUITE(move, loop_, *boost::unit_test::disabled())

BOOST_AUTO_TEST_CASE(create) {
  g_ctx().emplace<program_info>();
  g_reg()->ctx().emplace<image_to_move>(std::make_shared<detail::image_to_move>());
  auto l_h = entt::handle{*g_reg(), g_reg()->create()};
  FSys::path l_image_path{R"(D:\tmp\image_test_ep002_sc001)"};
  l_h.emplace<episodes>().analysis(l_image_path);
  l_h.emplace<shot>().analysis(l_image_path);
  l_h.emplace<image_to_move::element_type ::out_file_path>(l_image_path.parent_path());

  std::vector<FSys::path> l_files{FSys::list_files(l_image_path)};
  bool run_test{};
  auto l_w = boost::asio::make_work_guard(g_io_context());
  g_reg()->ctx().get<image_to_move>()->async_create_move(
      l_h, l_files,
      [l_r = &run_test, this, work = &l_w](FSys::path, boost::system::error_code) {
        *l_r = true;
        work->reset();
      }
  );

  g_io_context().run();
  BOOST_TEST(run_test);
}

BOOST_AUTO_TEST_SUITE_END()
