//
// Created by TD on 2022/9/29.
//
#include "doodle_core/core/app_base.h"
#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/work_task.h"
#include <doodle_core/doodle_core.h>
#include <doodle_core/json_rpc/core/server.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/platform/win/get_prot.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>
#include <doodle_app/app/this_rpc_exe.h>

#include "doodle_lib/distributed_computing/client.h"
#include <doodle_lib/app/doodle_main_app.h>
#include <doodle_lib/app/rpc_server_facet.h>
#include <doodle_lib/distributed_computing/server.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/static_thread_pool.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/process.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <entt/entity/fwd.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <iostream>
#include <main_fixtures/lib_fixtures.h>
#include <memory>
#include <string>
#include <vector>

using namespace doodle;
namespace {

struct loop_rpc {
  doodle_lib lib{};
};
}  // namespace

BOOST_FIXTURE_TEST_SUITE(rpc_server, loop_rpc, *boost::unit_test::disabled())

BOOST_AUTO_TEST_CASE(base) {
  bool run{true};
  distributed_computing::server l_s{};
  l_s.run();

  g_io_context().run();

  BOOST_TEST(run);
}

BOOST_AUTO_TEST_CASE(list_users) {
  bool run{true};
  for (auto i = 0u; i < 10; ++i) {
    auto l_h = make_handle();
    l_h.emplace<user>().set_name(fmt::format("user{}", i));
    l_h.emplace<database>();
  }

  distributed_computing::server l_s{};
  l_s.run();

  g_io_context().run();

  BOOST_TEST(run);
}

BOOST_AUTO_TEST_CASE(get_user_work_task_info) {
  bool run{true};
  std::vector<entt::handle> users{};

  auto l_main = users.emplace_back(make_handle());

  l_main.emplace<user>().set_name(fmt::format("user_{}", "main"s));
  l_main.get<user>().power = power_enum::modify_other_users;
  l_main.emplace<database>("19e0ed4f-0799-40b6-bf10-2a4c479c025e"s);
  for (auto j = 0u; j < 10; ++j) {
    auto l_h2 = make_handle();
    auto& l_w = l_h2.emplace<work_task_info>();
    l_w.user_ref.user_attr(l_main);
    l_w.task_name = fmt::format("work main_{} ", j);
    l_h2.emplace<database>();
  }
  for (auto i = 0u; i < 10; ++i) {
    auto l_h = users.emplace_back(make_handle());

    l_h.emplace<user>().set_name(fmt::format("user{}", i));
    l_h.emplace<database>();
    for (auto j = 0u; j < 10; ++j) {
      auto l_h2 = make_handle();
      auto& l_w = l_h2.emplace<work_task_info>();
      l_w.user_ref.user_attr(l_h);
      l_w.task_name = fmt::format("work {}_{} ", i, j);
      l_h2.emplace<database>();
    }
  }

  distributed_computing::server l_s{};
  l_s.run();

  g_io_context().run();

  BOOST_TEST(run);
}

BOOST_AUTO_TEST_CASE(new_user) {
  bool run{true};
  distributed_computing::server l_s{};

  for (auto&& l_work : l_s.socket_server_list) {
    l_work->register_sig([](const std::string& in) {
      auto l_all_reg_view1 = g_reg()->view<user>();
      if (in == "new.user") {
        BOOST_TEST(l_all_reg_view1.size() == 1);
      } else if (in == "set.user") {
        auto l_all_h1 = entt::handle{*g_reg(), l_all_reg_view1[0]};
        BOOST_TEST(l_all_h1.get<user>().get_name() == "tset_m"s);
        BOOST_TEST_MESSAGE(l_all_h1.get<user>());
      }
    });
  }

  l_s.run();

  g_io_context().run();

  BOOST_TEST(run);
}

BOOST_AUTO_TEST_SUITE_END()