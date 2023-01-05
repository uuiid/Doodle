//
// Created by TD on 2022/9/29.
//
#include "doodle_core/core/app_base.h"
#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/doodle_core_fwd.h"
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
#include <boost/test/unit_test_suite.hpp>

#include <entt/entity/fwd.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <iostream>
#include <main_fixtures/lib_fixtures.h>
#include <vector>

using namespace doodle;
struct loop_rpc {
  distributed_computing::server l_s{};
  loop_rpc() { l_s.run(); }
};
BOOST_FIXTURE_TEST_SUITE(rpc, loop_rpc)

BOOST_AUTO_TEST_CASE(base) {
  bool run{};
  boost::asio::post(g_thread(), [this, l_run = &run]() {
    distributed_computing::client l_c{};

    l_c.close();
    std::cout << "stop run"
              << "\n";
    l_s.work_guard->reset();
    g_io_context().stop();
    *l_run = true;
  });

  g_io_context().run();
  BOOST_TEST(run);
}

BOOST_AUTO_TEST_CASE(base_touch) {
  bool run{};
  boost::asio::post(g_thread(), [this, l_run = &run]() {
    distributed_computing::client l_c{};

    auto l_tset = l_c.touch("test");
    BOOST_TEST(l_tset == "success test");
    l_c.close();
    std::cout << "stop run"
              << "\n";
    l_s.work_guard->reset();
    g_io_context().stop();
    *l_run = true;
  });

  g_io_context().run();
  BOOST_TEST(run);
}

BOOST_AUTO_TEST_CASE(list_users) {
  for (auto i = 0u; i < 10; ++i) {
    auto l_h = make_handle();
    l_h.emplace<user>().set_name(fmt::format("user{}", i));
    l_h.emplace<database>();
  }

  boost::asio::post(g_thread(), [this]() {
    distributed_computing::client l_c{};
    auto l_users = l_c.list_users();
    BOOST_TEST(l_users.size() == 10);
    for (auto&& l_f : l_users) {
      std::cout << "user : " << fmt::to_string(l_f.get<user>()) << std::endl;
    }
    l_c.close();
    std::cout << "stop run"
              << "\n";
    l_s.work_guard->reset();
    g_io_context().stop();
  });
  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(get_user_work_task_info) {
  std::vector<entt::handle> users{};

  auto l_main = users.emplace_back(make_handle());

  l_main.emplace<user>().set_name(fmt::format("user_{}", "main"s));
  l_main.get<user>().power = power_enum::modify_other_users;
  l_main.emplace<database>();
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

  boost::asio::post(g_thread(), [this, l_main]() {
    distributed_computing::client l_c{};
    auto l_users = l_c.list_users();
    BOOST_TEST(l_users.size() == 11);
    for (auto&& l_f : l_users) {
      std::cout << "user : " << fmt::to_string(l_f.get<user>()) << std::endl;
      auto l_work_ = l_c.get_user_work_task_info(l_main, l_f);
      BOOST_TEST(l_work_.size() == 10);
      for (auto&& l_w : l_work_) {
        std::cout << "user : " << fmt::to_string(l_w.get<work_task_info>().task_name) << std::endl;
      }
    }
    l_c.close();
    std::cout << "stop run"
              << "\n";
    l_s.work_guard->reset();
    g_io_context().stop();
  });
  g_io_context().run();
}

BOOST_AUTO_TEST_SUITE_END()
