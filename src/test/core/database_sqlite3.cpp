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
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/export_file_info.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/move_create.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_task.h>
#include <doodle_core/pin_yin/convert.h>

#include <boost/test/unit_test.hpp>

#include "sqlpp11/all_of.h"
#include "sqlpp11/select.h"
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
using namespace doodle;
using namespace doodle::database_n;
BOOST_AUTO_TEST_CASE(test_sqlite3_create_table) {
  doodle_lib l_lib{};
  auto l_sql_conn = doodle_lib::Get().ctx().emplace<database_info>().get_connection();
  tables::work_task_info l_tables;
  l_sql_conn->execute(detail::create_table(l_tables)
                          .foreign_column(l_tables.entity_id, tables::entity{}.id)
                          .unique_column(l_tables.entity_id)
                          .end());
  l_sql_conn->execute(detail::create_index(l_tables.entity_id));
  l_sql_conn->execute(detail::create_index(l_tables.id));
  (*l_sql_conn)(sqlpp::select(sqlpp::all_of(l_tables)).from(l_tables).unconditionally());

  (*l_sql_conn)(sqlpp::sqlite3::drop_if_exists_table(l_tables));
}

void create_test_database() {
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::project>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::episodes>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::shot>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::season>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::assets>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::assets_file>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::time_point_wrap>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::comment>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::image_icon>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::importance>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::redirection_path_info>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::business::rules>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::user>();
  }
  {
    auto l_h = make_handle();
    l_h.emplace<doodle::database>();
    l_h.emplace<doodle::work_task_info>();
  }
}

BOOST_AUTO_TEST_CASE(test_sqlite3_save) {
  doodle_lib l_lib{};
  create_test_database();
  for (auto&& [e, i] : g_reg()->view<database>().each()) {
    BOOST_TEST_INFO(fmt::format("{}", i.uuid()));
  }

  g_reg()->ctx().get<file_translator_ptr>()->save_("D:/test.sqlite");
}

BOOST_AUTO_TEST_CASE(test_sqlite3_open) {
  FSys::remove("D:/test.sqlite");
  doodle_lib l_lib{};
  create_test_database();
  g_reg()->ctx().get<file_translator_ptr>()->save_("D:/test.sqlite");

  g_reg()->ctx().get<file_translator_ptr>()->open_("D:/test.sqlite");

  for (auto&& [e, i] : g_reg()->view<database>().each()) {
    BOOST_TEST_INFO(fmt::format("{}", i.uuid()));
  }
  BOOST_TEST_CHECK(g_reg()->view<database>().size() == 14);
}

BOOST_AUTO_TEST_CASE(test_sqlite3_old_open_save) {
  FSys::copy(
      R"(D:/test_file/cloth_test/JG_back_up.doodle_db)", "D:/test_file/cloth_test/JG.doodle_db",
      FSys::copy_options::overwrite_existing
  );
  doodle_lib l_lib{};

  g_reg()->ctx().get<file_translator_ptr>()->open_("D:/test_file/cloth_test/JG.doodle_db");

  for (auto&& [e, i] : g_reg()->view<database>().each()) {
    BOOST_TEST_INFO(fmt::format("{}", i.uuid()));
  }
  g_reg()->ctx().get<file_translator_ptr>()->save_("D:/test_file/cloth_test/JG2.doodle_db");
  g_reg()->ctx().get<file_translator_ptr>()->open_("D:/test_file/cloth_test/JG2.doodle_db");
}