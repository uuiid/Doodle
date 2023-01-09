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
#include <vector>

using namespace doodle;
struct loop_rpc {
  doodle_lib lib{};

  loop_rpc() {}
};

namespace doodle {
std::ostream& boost_test_print_type(std::ostream& ostr, database const& right) {
  ostr << "id: " << right.get_id() << " uuid: " << right.uuid();

  return ostr;
}
std::ostream& boost_test_print_type(std::ostream& ostr, user const& right) {
  ostr << "name: " << right.get_name() << "(" << right.get_enus() << ")";

  return ostr;
}
}  // namespace doodle

BOOST_FIXTURE_TEST_SUITE(rpc_client, loop_rpc)

BOOST_AUTO_TEST_CASE(base) {
  bool run{true};
  distributed_computing::client l_c{};

  l_c.close();
  BOOST_TEST_MESSAGE("stop run");

  g_io_context().run();
  BOOST_TEST(run);
}

BOOST_AUTO_TEST_CASE(list_fun1) {
  distributed_computing::client l_c{};

  auto l_tset = l_c.list_fun();
  for (auto&& i : l_tset) {
    BOOST_TEST_MESSAGE(i);
  }
  l_c.close();

  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(list_users) {
  distributed_computing::client l_c{};
  auto l_users = l_c.list_users();
  BOOST_TEST(l_users.size() == 10);
  for (auto&& l_f : l_users) {
    std::cout << "user : " << fmt::to_string(l_f.get<user>()) << std::endl;
  }
  l_c.close();

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

    g_io_context().stop();
  });
  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(new_user) {
  bool l_run{};
  std::vector<entt::handle> users{};

  auto l_main = users.emplace_back(make_handle());

  l_main.emplace<user>().set_name(fmt::format("user_{}", "main"s));
  l_main.get<user>().power = power_enum::modify_other_users;
  l_main.emplace<database>();

  boost::asio::post(g_thread(), [this, l_main, l_ptr_run = &l_run]() {
    auto l_reg = std::make_shared<entt::registry>();
    distributed_computing::client l_c{l_reg};
    /// 创建新的用户
    auto l_users   = l_c.new_user(user{"tset"s});
    auto l_user_v1 = l_reg->view<user>();

    BOOST_TEST(l_user_v1.size() == 1);
    BOOST_TEST((l_users.all_of<database, user>()));
    BOOST_TEST(l_users.get<user>().get_name() == "tset"s);
    BOOST_TEST(!l_users.get<database>().uuid().is_nil());
    auto l_all_reg_view1 = g_reg()->view<user>();
    BOOST_TEST(l_all_reg_view1.size() == 2);
    BOOST_TEST_MESSAGE(l_users.get<user>());
    auto l_uuid = l_users.get<database>().uuid();
    l_reg->clear();

    /// 登录用户
    auto l_user_g        = l_c.get_user(l_uuid);
    auto& l_user_current = l_reg->ctx().emplace<user::current_user>();
    l_user_current.set_user(l_user_g);

    auto l_user_v2 = l_reg->view<user>();

    BOOST_TEST(l_user_v2.size() == 1);
    BOOST_TEST(l_user_g.get<database>() == l_users.get<database>());
    BOOST_TEST(l_user_g.get<user>() == l_users.get<user>());
    BOOST_TEST(l_all_reg_view1.size() == 2);
    /// 设置用户
    l_user_g.get<user>().set_name("tset_m");
    l_c.set_user(l_user_g);

    auto l_user_v3 = l_reg->view<user>();

    BOOST_TEST(l_user_v3.size() == 1);
    auto l_h1 = entt::handle{*g_reg(), l_user_v3[0]};
    BOOST_TEST(l_h1.get<user>().get_name() == "tset_m"s);
    BOOST_TEST(l_all_reg_view1.size() == 2);
    auto l_all_h1 = entt::handle{*g_reg(), l_all_reg_view1[0]};
    BOOST_TEST(l_all_h1.get<user>().get_name() == "tset_m"s);
    BOOST_TEST_MESSAGE(l_h1.get<user>());
    BOOST_TEST_MESSAGE(l_all_h1.get<user>());
    /// 关闭
    l_c.close();

    g_io_context().stop();
    *l_ptr_run = true;
  });
  g_io_context().run();
  BOOST_TEST(l_run);
}

BOOST_AUTO_TEST_SUITE_END()
