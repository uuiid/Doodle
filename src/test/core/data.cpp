//
// Created by TD on 24-9-24.
//

#include "doodle_core/metadata/entity.h"

#include "doodle_lib/core/global_function.h"
#include "doodle_lib/http_client/kitsu_client.h"
#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/http_client_core.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/sqlite_orm/detail/dynamic_where.h>
#include <doodle_lib/sqlite_orm/detail/uuid_to_blob.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

#include <sqlite_orm/sqlite_orm.h>

BOOST_AUTO_TEST_SUITE(data)
BOOST_AUTO_TEST_CASE(http_client) {
  using namespace doodle;
  app_base l_app{};
  kitsu::kitsu_client l_client{"http://127.0.0.1:50025"};

  boost::asio::co_spawn(
      g_io_context(),
      [&]() -> boost::asio::awaitable<void> {
        auto l_uuid = from_uuid_str("96a1f1d5-e37d-4f22-90e0-1817468c9c3e");
        co_await l_client.upload_asset_file_maya(
            l_uuid, "D:\\test_db\\public\\DYX\\6_moxing\\CFX\\Ch014B_rig_jxh_cloth.ma"
        );
      },
      boost::asio::detached
  );
  l_app.run();
}

BOOST_AUTO_TEST_CASE(sqlite_orm_dynamic_where) {
  using namespace doodle;
  using namespace sqlite_orm;
  app_base l_app{};
  auto l_sql = make_storage(
      "", make_table(
              "entity", make_column("uuid_id", &entity::uuid_id_), make_column("name", &entity::name_),
              make_column("entity_type_id", &entity::entity_type_id_)
          )
  );
  l_sql.open_forever();
  l_sql.sync_schema();

  auto l_uuid       = from_uuid_str("96a1f1d5-e37d-4f22-90e0-1817468c9c3e");

  auto l_pr_not_dyn = l_sql.prepare(
      select(&entity::uuid_id_, from<entity>(), where(c(&entity::uuid_id_) == l_uuid && c(&entity::name_) == "test"))
  );
  auto l_sql_str_not_dyn = l_pr_not_dyn.sql();
  auto l_dynamic_where = dynamic_where(l_sql);
  l_dynamic_where.push_back(c(&entity::uuid_id_) == l_uuid);
  l_dynamic_where.push_back(c(&entity::name_) == "test");
  auto l_select  = select(&entity::uuid_id_, from<entity>(), where(l_dynamic_where));
  auto l_pr      = l_sql.prepare(l_select);
  auto l_sql_str = l_pr.sql();
}

BOOST_AUTO_TEST_SUITE_END()