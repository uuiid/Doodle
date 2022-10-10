//
// Created by TD on 2022/9/29.
//
#include <main_fixtures/lib_fixtures.h>

#include <boost/test/unit_test.hpp>

#include <doodle_core/doodle_core.h>
#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>
#include <doodle_lib/app/doodle_main_app.h>
#include <doodle_lib/app/rpc_server_facet.h>
#include <doodle_lib/long_task/image_to_move.h>
#include <doodle_core/json_rpc/core/server.h>
#include <doodle_core/platform/win/get_prot.h>

#include <doodle_app/app/this_rpc_exe.h>

#include <boost/process.hpp>

namespace doodle {

class test_this_exe : public details::image_to_move {
 public:
 private:
  void create_move(const FSys::path& in_out_path, process_message& in_msg, const std::vector<image_attr>& in_vector) override {
    detail::this_rpc_exe l_exe{};
    l_exe.create_move(in_out_path, in_vector, in_msg);
  }
};

class test_app2 : public doodle_main_app {
 public:
  test_app2() : doodle::doodle_main_app() {
    f_attr    = std::make_shared<facet::rpc_server_facet>();
    run_facet = f_attr;
    add_facet(run_facet);
  }
  std::shared_ptr<facet::rpc_server_facet> f_attr{};
};
}  // namespace doodle

using namespace doodle;

struct loop_rpc {
  ::doodle::test_app2 main_app_attr{};
  std::shared_ptr<boost::asio::high_resolution_timer> timer;
  std::function<void()> run_fun;
  bool is_run{};
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work{
      boost::asio::make_work_guard(g_io_context())};
  loop_rpc() {
    //    work = std::move(boost::asio::make_work_guard(g_io_context()));
    timer = std::make_shared<boost::asio::high_resolution_timer>(g_io_context());
    timer->expires_from_now(1s);
    timer->async_wait([this](boost::system::error_code) {
      run_fun();
      is_run = true;
    });
  }
};

BOOST_FIXTURE_TEST_SUITE(rpc, loop_rpc)
BOOST_AUTO_TEST_CASE(base) {
  bool run_test{};

  //  auto l_prot   = main_app_attr.f_attr->server_attr()->get_prot();
  //  auto l_f_prot = win::get_tcp_port(boost::this_process::get_id());
  //  BOOST_TEST(l_prot == l_f_prot);
  run_fun = []() { app_base::Get().stop_app(); };
  main_app_attr.run();
  BOOST_TEST(is_run);
}

BOOST_AUTO_TEST_CASE(sub_exe) {
  run_fun = [&]() {
    auto l_h = make_handle();
    FSys::path l_image_path{R"(E:\tmp\image_test_ep002_sc001)"};
    l_h.emplace<episodes>().analysis(l_image_path);
    l_h.emplace<shot>().analysis(l_image_path);
    l_h.emplace<FSys::path>(l_image_path.parent_path());

    std::vector<FSys::path> l_files{FSys::list_files(l_image_path)};

    g_reg()->ctx().emplace<image_to_move>() = std::make_shared<test_this_exe>();
    g_reg()->ctx().at<image_to_move>()->async_create_move(
        l_h, l_files, [&]() {
          work.reset();
          timer->async_wait([](boost::system::error_code) {
            app_base::Get().stop_app();
          });
          timer->expires_from_now(2s);
        }
    );
  };
  main_app_attr.run();
  BOOST_TEST(is_run);
}

BOOST_AUTO_TEST_SUITE_END()
