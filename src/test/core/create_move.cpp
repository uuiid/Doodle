//
// Created by TD on 2022/10/8.
//

#include <doodle_core/core/program_info.h>
#include <doodle_core/doodle_core.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>
#include <doodle_app/app/this_rpc_exe.h>

#include <doodle_lib/facet/main_facet.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <boost/test/unit_test.hpp>

#include <crtdbg.h>
#include <main_fixtures/lib_fixtures.h>
#include <stdlib.h>

using namespace doodle;

namespace {
struct loop_ {
  doodle_lib lib{};
};
}  // namespace
BOOST_FIXTURE_TEST_SUITE(move, loop_, *boost::unit_test::disabled())

BOOST_AUTO_TEST_CASE(create) {
  doodle_lib::Get().ctx().emplace<program_info>();
  g_reg()->ctx().emplace<image_to_move>(std::make_shared<detail::image_to_move>());
  auto l_h = make_handle();
  FSys::path l_image_path{R"(D:\tmp\image_test_ep002_sc001)"};
  l_h.emplace<episodes>().analysis(l_image_path);
  l_h.emplace<shot>().analysis(l_image_path);
  l_h.emplace<FSys::path>(l_image_path.parent_path());

  std::vector<FSys::path> l_files{FSys::list_files(l_image_path)};
  bool run_test{};
  auto l_w = boost::asio::make_work_guard(g_io_context());
  g_reg()->ctx().at<image_to_move>()->async_create_move(l_h, l_files, [l_r = &run_test, this, work = &l_w]() {
    *l_r = true;
    work->reset();
  });

  g_io_context().run();
  BOOST_TEST(run_test);
}

class maya_create_movie : public doodle::detail::image_to_movie_interface {
  detail::this_rpc_exe run{};

 public:
  maya_create_movie()          = default;
  virtual ~maya_create_movie() = default;

  void create_move(const FSys::path& in_out_path, process_message& in_msg, const std::vector<image_attr>& in_vector)
      override {
    DOODLE_LOG_INFO("开始doodle 进程合成视频");
    run.create_move(in_out_path, in_vector, in_msg);
  };

 protected:
  FSys::path create_out_path(const entt::handle& in_handle) override {
    boost::ignore_unused(this);

    FSys::path l_out{};
    l_out = in_handle.get<FSys::path>();

    /// \brief 这里我们检查 shot，episode 进行路径的组合
    if (!l_out.has_extension() && in_handle.any_of<episodes, shot>())
      l_out /= fmt::format(
          "{}_{}.mp4", in_handle.any_of<episodes>() ? fmt::to_string(in_handle.get<episodes>()) : "eps_none"s,
          in_handle.any_of<shot>() ? fmt::to_string(in_handle.get<shot>()) : "sh_none"s
      );
    else if (!l_out.has_extension()) {
      l_out /= fmt::format("{}.mp4", core_set::get_set().get_uuid());
    } else
      l_out.extension() == ".mp4" ? void() : throw_exception(doodle_error{"扩展名称不是MP4"});

    if (exists(l_out.parent_path())) create_directories(l_out.parent_path());
    return l_out;
  };
};

BOOST_AUTO_TEST_CASE(sub_create) {
  doodle_lib::Get().ctx().emplace<program_info>();
  auto l_exe = std::make_shared<maya_create_movie>();
  auto l_h   = make_handle();
  FSys::path l_image_path{R"(D:\tmp\image_test_ep002_sc001)"};
  l_h.emplace<episodes>().analysis(l_image_path);
  l_h.emplace<shot>().analysis(l_image_path);
  l_h.emplace<FSys::path>(l_image_path.parent_path());

  bool run_test{};

  l_exe->async_create_move(
      l_h, FSys::list_files(l_image_path),
      [l_exe, l_r = &run_test, this, work = boost::asio::make_work_guard(g_io_context())]() { *l_r = true; }
  );
  g_io_context().run();
  BOOST_TEST(FSys::exists("D:/tmp/ep_2_sc_1.mp4"));
  BOOST_TEST(run_test);
}

BOOST_AUTO_TEST_SUITE_END()
