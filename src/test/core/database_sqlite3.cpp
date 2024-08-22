//
// Created by TD on 2023/1/13.
//
//
// Created by TD on 2022/8/26.
//

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/database_task/sqlite_client.h"
#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_snapshot.h>

#include "doodle_app/app/app_command.h"

#include <boost/test/unit_test.hpp>

#include "sqlpp11/all_of.h"
#include "sqlpp11/select.h"
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
using namespace doodle;
using namespace doodle::database_n;

void create_test_database() {
  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::project>();
  }
  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::episodes>();
  }
  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::shot>(100, shot::shot_ab_enum::A);
  }
  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::season>();
  }
  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::assets>();
  }
  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::assets_file>();
  }
  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::time_point_wrap>();
  }
  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::comment>();
  }
  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::image_icon>();
  }
  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::importance>();
  }
  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::business::rules>();
  }
  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::user>("test_user");
  }
  for (int l = 0; l < 10; ++l)

  {
    auto l_h = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<doodle::database>();
    auto& l_s            = l_h.emplace<doodle::server_task_info>();
    l_s.name_            = fmt::format("name_{}", l);
    l_s.source_computer_ = fmt::format("source_computer_{}", l);
    l_s.submitter_       = fmt::format("submitter_{}", l);
    l_s.run_computer_    = fmt::format("run_computer_{}", l);
  }
}

BOOST_AUTO_TEST_CASE(test_sqlite3_save) {
  doodle_lib l_lib{};
  g_ctx().get<file_translator_ptr>()->new_file_scene(database_info::memory_data, project{});
  create_test_database();
  g_ctx().emplace<program_info>();
  g_ctx().get<file_translator_ptr>()->save();
  for (auto&& [e, i] : g_reg()->view<database>().each()) {
    BOOST_TEST_INFO(fmt::format("{}", i.uuid()));
  }
}

BOOST_AUTO_TEST_CASE(test_sqlite3_open) {
  FSys::remove("D:/test.sqlite");
  doodle_lib l_lib{};
  g_ctx().get<file_translator_ptr>()->async_open(FSys::path{}, false, true, g_reg(), [](auto&&) {});
  create_test_database();

  g_ctx().get<file_translator_ptr>()->async_open("D:/test.sqlite", false, true, g_reg(), [](auto&&) {});
  BOOST_TEST_CHECK(g_reg()->view<database>().size() == 14);
  BOOST_TEST_CHECK(g_reg()->view<user>().size() == 1);
  BOOST_TEST_CHECK(g_reg()->view<shot>().size() == 1);

  for (auto&& [e, i] : g_reg()->view<database>().each()) {
    BOOST_TEST_INFO(fmt::format("{}", i.uuid()));
  }

  for (auto&& [e, i] : g_reg()->view<user>().each()) {
    BOOST_TEST_CHECK(i.get_name() == "test_user");
  }

  for (auto&& [e, i] : g_reg()->view<shot>().each()) {
    BOOST_TEST_CHECK(i.get_shot() == 100ll);
  }
}

BOOST_AUTO_TEST_CASE(test_sqlite3_old_open_save) {
  app_command<> l_App{};

  g_ctx().get<file_translator_ptr>()->async_open(
      "D:/test_file/test_db/10_texiao.doodle_db", false, true, g_reg(), [](auto&&) {}
  );

  for (auto&& [e, i] : g_reg()->view<database>().each()) {
    BOOST_TEST_INFO(fmt::format("{}", i.uuid()));
  }
  g_ctx().get<file_translator_ptr>()->async_open(
      "D:/test_file/cloth_test/JG2.doodle_db", false, true, g_reg(), [](auto&&) {}
  );
}

BOOST_AUTO_TEST_CASE(test_sqlite3_snapshot) {
  app_command<> l_App{};
  create_test_database();
  snapshot::sqlite_snapshot l_snap{"D:/test.db", *g_reg()};
  l_snap.save<database, server_task_info>();
  std::vector<std::pair<entt::entity, boost::uuids::uuid>> l_list{};
  std::vector<std::pair<entt::entity, server_task_info>> l_list_s{};
  for (auto&& [e, i] : g_reg()->view<database>().each()) {
    BOOST_TEST_INFO(fmt::format("{} {}", e, i.uuid()));
    l_list.emplace_back(e, i.uuid());
  }
  for (auto&& [e, i] : g_reg()->view<server_task_info>().each()) {
    l_list_s.emplace_back(e, i);
  }
  entt::registry l_reg{};
  snapshot::sqlite_snapshot l_snap2{"D:/test.db", l_reg};
  l_snap2.load<database, server_task_info>();
  std::vector<std::pair<entt::entity, boost::uuids::uuid>> l_list2{};
  std::vector<std::pair<entt::entity, server_task_info>> l_list_s2{};
  for (auto&& [e, i] : l_reg.view<database>().each()) {
    BOOST_TEST_INFO(fmt::format("{} {}", e, i.uuid()));
    l_list2.emplace_back(e, i.uuid());
  }
  for (auto&& [e, i] : l_reg.view<server_task_info>().each()) {
    l_list_s2.emplace_back(e, i);
  }
  BOOST_TEST_CHECK(l_list == l_list2);
  BOOST_TEST_CHECK(l_list_s == l_list_s2);
}

BOOST_AUTO_TEST_CASE(test_sqlite3_multi_thread) {
  app_command<> l_App{};
  l_App.use_multithread(true);
  g_pool_db().set_path("D:/test.db");
  {
    auto L_con = g_pool_db().get_connection();
    server_task_info::create_table(L_con);
  }
  std::vector<server_task_info> l_list{};
  l_list.reserve(1000000);
  for (int i = 0; i < 1000000; ++i) {
    l_list.emplace_back(server_task_info{core_set::get_set().get_uuid(), "tset.exe", {"dasdas", "daaaa", "adsdasds"}});
  }
  auto l_s = boost::asio::make_strand(g_io_context());
  for (int i = 0; i < 1000000; ++i) {
    boost::asio::post(l_s, [i, &l_list]() {
      auto l_conn = g_pool_db().get_connection();
      auto l_c    = sqlpp::start_transaction(l_conn);
      if (i % 2 == 0) {
        server_task_info::insert(l_conn, {l_list[i]});
      } else {
        server_task_info::delete_by_ids(l_conn, {l_list[i].id_});
      }
      l_c.commit();
    });
  }

  for (int i = 0; i < 1000000; ++i) {
    boost::asio::post(l_s, [i, &l_list]() {
      auto l_conn = g_pool_db().get_connection();
      auto l_c    = sqlpp::start_transaction(l_conn);

      if (i % 2 != 0) {
        server_task_info::insert(l_conn, {l_list[i]});
      } else {
        server_task_info::delete_by_ids(l_conn, {l_list[i].id_});
      }
      l_c.commit();
    });
  }
  boost::asio::post(l_s, []() { app_base::Get().stop_app(); });
  l_App.run();
}