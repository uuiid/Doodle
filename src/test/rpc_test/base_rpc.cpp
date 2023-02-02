//
// Created by TD on 2022/9/29.
//
#include "doodle_core/core/app_base.h"
#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
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
#include <doodle_lib/distributed_computing/server.h>
#include <doodle_lib/facet/main_facet.h>
#include <doodle_lib/facet/rpc_server_facet.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/static_thread_pool.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/process.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/uuid/uuid.hpp>

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
  run_subprocess l_sub{g_io_context()};
};

BOOST_FIXTURE_TEST_SUITE(rpc_client, loop_rpc)

BOOST_AUTO_TEST_CASE(base) {
  bool run{true};
  l_sub.run("rpc_server/base");
  distributed_computing::client l_c{"127.0.0.1"};

  l_c.close();
  BOOST_TEST_MESSAGE("stop run");

  g_io_context().run();
  BOOST_TEST(run);
}

BOOST_AUTO_TEST_CASE(list_fun1) {
  distributed_computing::client l_c{"127.0.0.1"};
  l_sub.run("rpc_server/base");

  auto l_tset = l_c.list_fun();
  for (auto&& i : l_tset) {
    BOOST_TEST_MESSAGE(i);
  }
  l_c.close();

  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(list_users) {
  distributed_computing::client l_c{"127.0.0.1"};
  l_sub.run("rpc_server/list_users");

  auto l_users = l_c.list_users();
  BOOST_TEST(l_users.size() == 10);
  for (auto&& l_f : l_users) {
    std::cout << "user : " << fmt::to_string(l_f.get<user>()) << std::endl;
  }
  l_c.close();

  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(new_user) {
  l_sub.run("rpc_server/new_user");

  auto l_reg = std::make_shared<entt::registry>();
  distributed_computing::client l_c{l_reg, "127.0.0.1"};
  /// 创建新的用户
  auto l_users   = l_c.new_user(user{"tset"s});
  auto l_user_v1 = l_reg->view<user>();

  BOOST_TEST(l_user_v1.size() == 1);
  BOOST_TEST((l_users.all_of<database, user>()));
  BOOST_TEST(l_users.get<user>().get_name() == "tset"s);
  BOOST_TEST(!l_users.get<database>().uuid().is_nil());

  BOOST_TEST_MESSAGE(l_users.get<user>());

  /// 关闭
  l_c.close();

  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(get_user) {
  l_sub.run("rpc_server/get_user");

  auto l_reg = std::make_shared<entt::registry>();
  distributed_computing::client l_c{l_reg, "127.0.0.1"};

  auto l_user_g = l_c.get_user(boost::lexical_cast<boost::uuids::uuid>("19e0ed4f-0799-40b6-bf10-2a4c479c025e"s));

  BOOST_TEST(l_user_g.get<user>().get_name() == "test1");
  BOOST_TEST(
      l_user_g.get<database>().uuid() ==
      boost::lexical_cast<boost::uuids::uuid>("19e0ed4f-0799-40b6-bf10-2a4c479c025e"s)
  );

  BOOST_TEST_MESSAGE(l_user_g.get<user>());
  /// 关闭
  l_c.close();

  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(get_user_f) {
  l_sub.run("rpc_server/get_user");

  auto l_reg = std::make_shared<entt::registry>();
  distributed_computing::client l_c{l_reg, "127.0.0.1"};

  BOOST_CHECK_THROW(
      (l_c.get_user(boost::lexical_cast<boost::uuids::uuid>("19e0ed4f-0799-40b6-bf10-2a4c479c0251"s))),
      json_rpc::invalid_id_exception
  );
  /// 关闭
  l_c.close();

  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(set_user) {
  l_sub.run("rpc_server/set_user");

  auto l_reg = std::make_shared<entt::registry>();
  distributed_computing::client l_c{l_reg, "127.0.0.1"};

  auto l_user_g = l_c.get_user(boost::lexical_cast<boost::uuids::uuid>("19e0ed4f-0799-40b6-bf10-2a4c479c025e"s));

  l_reg->ctx().emplace<user::current_user>().set_user(l_user_g);

  BOOST_TEST(l_user_g.get<user>().get_name() == "test1");
  BOOST_TEST(
      l_user_g.get<database>().uuid() ==
      boost::lexical_cast<boost::uuids::uuid>("19e0ed4f-0799-40b6-bf10-2a4c479c025e"s)
  );
  l_user_g.get<user>().set_name("tset_m");
  l_c.set_user(l_user_g);

  BOOST_TEST_MESSAGE(l_user_g.get<user>());
  /// 关闭
  l_c.close();

  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(get_user_work_task_info) {
  l_sub.run("rpc_server/get_user_work_task_info");

  auto l_main = make_handle();
  l_main.emplace<database>("19e0ed4f-0799-40b6-bf10-2a4c479c025e"s);

  distributed_computing::client l_c{"127.0.0.1"};
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

  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(set_user_work_task_info) {
  l_sub.run("rpc_server/set_user_work_task_info");

  distributed_computing::client l_c{"127.0.0.1"};
  auto l_users = l_c.get_user(boost::lexical_cast<boost::uuids::uuid>("19e0ed4f-0799-40b6-bf10-2a4c479c025e"s));

  g_reg()->ctx().emplace<user::current_user>().set_user(l_users);

  auto l_whs = l_c.get_user_work_task_info(l_users, l_users);
  BOOST_TEST(l_whs.size() == 1);

  auto& l_work_com = l_whs[0].get<work_task_info>();

  BOOST_TEST(l_work_com.task_name == "clict_set_s1");
  BOOST_TEST(l_work_com.abstract == "clict_set_s2");
  BOOST_TEST(l_work_com.region == "clict_set_s3");

  l_work_com.task_name = "clict_set_test1";
  l_work_com.abstract  = "clict_set_test2";
  l_work_com.region    = "clict_set_test3";
  l_work_com.time      = chrono::round<chrono::hours>(time_point_wrap{2022, 12, 1}.get_local_time());

  l_c.set_work_task_info(l_users, l_whs[0]);

  BOOST_TEST(l_work_com.task_name == "clict_set_test1");
  BOOST_TEST(l_work_com.abstract == "clict_set_test2");
  BOOST_TEST(l_work_com.region == "clict_set_test3");

  l_c.close();

  g_io_context().run();
}

BOOST_AUTO_TEST_SUITE_END()
