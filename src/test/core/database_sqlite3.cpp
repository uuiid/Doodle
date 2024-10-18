//
// Created by TD on 2023/1/13.
//
//
// Created by TD on 2022/8/26.
//

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/database_task/sqlite_client.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"
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
#include <doodle_core/metadata/scan_data_t.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/detail/assets_type_enum.h>
#include <doodle_core/sqlite_orm/sqlite_snapshot.h>
#include <doodle_core/sqlite_orm/detail/std_filesystem_path_orm.h>
#include <doodle_core/sqlite_orm/detail/uuid_to_blob.h>

#include "doodle_app/app/app_command.h"

#include <boost/test/unit_test.hpp>

#include "sqlpp11/all_of.h"
#include "sqlpp11/select.h"
#include <sqlite_orm/sqlite_orm.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
using namespace doodle;
using namespace doodle::database_n;

BOOST_AUTO_TEST_SUITE(sql_)

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

BOOST_AUTO_TEST_CASE(test_sqlite3_orm) {
  using namespace sqlite_orm;
  auto l_s = make_storage(
      "D:/test.db",  //
      make_table(
          "project_tab",                                                       //
          make_column("id", &project_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &project_helper::database_t::uuid_id_, unique()),
          make_column("name", &project_helper::database_t::name_),      //
          make_column("en_str", &project_helper::database_t::en_str_),  //
          make_column("shor_str", &project_helper::database_t::shor_str_),
          make_column("local_path", &project_helper::database_t::local_path_),
          make_column("auto_upload_path", &project_helper::database_t::auto_upload_path_)
      )
  );
  l_s.sync_schema(true);

  project_helper::database_t l_d1{
      .id_               = 6,
      .uuid_id_          = core_set::get_set().get_uuid(),
      .name_             = "test",
      .en_str_           = "test",
      .shor_str_         = "test",
      .local_path_       = "test",
      .auto_upload_path_ = "test"
  };
  project_helper::database_t l_d2{
      .uuid_id_          = core_set::get_set().get_uuid(),
      .name_             = "test2",
      .en_str_           = "test2",
      .shor_str_         = "test2",
      .local_path_       = "test2",
      .auto_upload_path_ = "test2"
  };

  project_helper::database_t l_d3{
      .id_               = 6,
      .uuid_id_          = core_set::get_set().get_uuid(),
      .name_             = "test3",
      .en_str_           = "test3",
      .shor_str_         = "test3",
      .local_path_       = "test3",
      .auto_upload_path_ = "test3"
  };
  l_s.replace(l_d3);
  l_d3.name_ = "test4";
  l_s.replace(l_d3);
}

// 多线程测试
BOOST_AUTO_TEST_CASE(multi_threaded) {
  app_command<> l_app{};
  l_app.use_multithread(true);

  auto l_data = std::make_shared<project_helper::database_t>(project_helper::database_t{
      .uuid_id_          = core_set::get_set().get_uuid(),
      .name_             = "das",
      .path_             = "122",
      .en_str_           = "333",
      .shor_str_         = "323",
      .local_path_       = "dddd",
      .auto_upload_path_ = "323"
  });
  auto l_list = std::make_shared<std::vector<scan_data_t::database_t>>(
      100,
      scan_data_t::database_t{
          .ue_path_    = "das",
          .rig_path_   = "das",
          .solve_path_ = "dsadssa",
          .name_       = "name",
          .version_    = "sda",
          .num_        = "das"
      }
  );
  for (auto&& i : *l_list) {
    i.uuid_id_    = core_set::get_set().get_uuid();
    i.ue_uuid_    = core_set::get_set().get_uuid();
    i.ue_path_    = "test";
    i.rig_path_   = "test";
    i.rig_uuid_   = core_set::get_set().get_uuid();
    i.solve_path_ = "test";
    i.solve_uuid_ = core_set::get_set().get_uuid();
  }
  g_ctx().emplace<sqlite_database>().load("D:/test2.db");
  boost::asio::post(g_io_context(), [&]() mutable {
    auto l_prj_id =
        boost::asio::co_spawn(g_io_context(), g_ctx().get<sqlite_database>().install(l_data), boost::asio::use_future)
            .get();

    auto l_ids = boost::asio::co_spawn(
                     g_io_context(), g_ctx().get<sqlite_database>().install_range(l_list), boost::asio::use_future
    )
                     .get();

    boost::asio::co_spawn(g_io_context(), g_ctx().get<sqlite_database>().install_range(l_list), boost::asio::use_future)
        .get();
    for (auto&& i : *l_list) {
      boost::asio::co_spawn(
          g_io_context(), g_ctx().get<sqlite_database>().install(std::make_shared<scan_data_t::database_t>(i)),
          boost::asio::detached
      );
    }
    for (auto&& i : *l_list)
      boost::asio::post(g_io_context(), [id_ = i.ue_uuid_]() {
        g_ctx().get<sqlite_database>().get_by_uuid<scan_data_t::database_t>(*id_);
      });
  });
  l_app.run();
}

struct test_1 {
  std::int32_t id_{};
  std::int32_t id_fk_{};
};

struct test_2 {
  std::int32_t id_{};
  std::string tt1{};
};

auto l_mk() {
  using namespace sqlite_orm;
  return make_storage(
      "C:/tes.db",  //
      make_table(
          "test_tab2",  //
          make_column("id", &test_1::id_, primary_key()), make_column("fk", &test_1::id_fk_),
          foreign_key(&test_1::id_fk_).references(&test_2::id_)
      ),

      make_table(
          "test_tab",  //
          make_column("id", &test_2::id_, primary_key()),
          make_column("tt1", &test_2::tt1, null())
      )
  );
}

BOOST_AUTO_TEST_CASE(tset_null) {
  {
    auto l_s = l_mk();
    l_s.sync_schema(true);
    auto l_id = l_s.insert(test_2{});
    l_s.insert(test_1{.id_fk_ = l_id});
  }
  auto l_s = l_mk();
  l_s.sync_schema(true);  // 这里出现错误

}

BOOST_AUTO_TEST_SUITE_END()