//
// Created by TD on 24-9-24.
//

#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/entity_type.h"

#include "doodle_lib/core/core_set.h"
#include "doodle_lib/core/global_function.h"
#include "doodle_lib/http_client/kitsu_client.h"
#include "doodle_lib/sqlite_orm/orm/update.h"
#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/http_client_core.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/sqlite_orm/detail/dynamic_where.h>
#include <doodle_lib/sqlite_orm/detail/uuid_to_blob.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <ratio>
#include <sqlite_orm/sqlite_orm.h>
#include <utility>
#include <vector>

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

  auto l_uuid        = from_uuid_str("96a1f1d5-e37d-4f22-90e0-1817468c9c3e");
  auto l_uuid_vector = std::vector<uuid>{l_uuid};

  auto l_pr_not_dyn  = l_sql.prepare(
      select(&entity::uuid_id_, from<entity>(), where(c(&entity::uuid_id_) == l_uuid && c(&entity::name_) == "test"))
  );
  auto l_sql_str_not_dyn = l_pr_not_dyn.sql();
  auto l_dynamic_where   = dynamic_where(l_sql);
  l_dynamic_where.push_back(not_in(&entity::uuid_id_, l_uuid_vector));
  l_dynamic_where.push_back(c(&entity::name_) == "test");
  auto l_select  = select(&entity::uuid_id_, from<entity>(), where(l_dynamic_where));
  auto l_pr      = l_sql.prepare(l_select);
  auto l_sql_str = l_pr.sql();
  l_sql.select(&entity::uuid_id_, from<entity>(), where(l_dynamic_where));
}
BOOST_AUTO_TEST_CASE(mu_sqlorm_type_id) { using namespace doodle::orm; }
BOOST_AUTO_TEST_CASE(mu_sqlorm) {
  using namespace doodle;
  using namespace doodle::orm;
  auto l_reg = orm::storage{};
  //   auto l_enit_tab = orm::make_table_info<entity>("entity");
  //   using test_t    = std::decay_t<decltype(&entity::name_)>;
  //   l_enit_tab.add_column("id", &entity::id_, orm::primary_key(), orm::autoincrement(), orm::not_null())
  //       .add_column("uuid_id", &entity::uuid_id_)
  //       .add_column("name", &entity::name_)
  //       .add_foreign_key(
  //           &entity::entity_type_id_, &asset_type::uuid_id_, orm::on_delete(orm::foreign_key_action::cascade)
  //       );
  //   ;

  l_reg.reg_table<entity>("entity")
      .add_column("id", &entity::id_, orm::primary_key(), orm::autoincrement(), orm::not_null())
      .add_column("uuid", &entity::uuid_id_)
      .add_column("name", &entity::name_)
      .add_column("entity_type_id", &entity::entity_type_id_)
      .add_foreign_key(
          "entity_type_id", &entity::entity_type_id_, &asset_type::uuid_id_, orm::foreign_key_action::cascade
      )
      .add_index("entity_uuid_id_index", &entity::uuid_id_)
      .add_unique_index("entity_name_unique_index", &entity::name_);
  ;

  l_reg.reg_table<asset_type>("asset_type")
      .add_column("id", &asset_type::id_, orm::primary_key(), orm::autoincrement(), orm::not_null())
      .add_column("uuid", &asset_type::uuid_id_)
      .add_column("name", &asset_type::name_);
  l_reg.finalize();
  l_reg.open();
  l_reg.sync_schema();
  auto l_uuid           = from_uuid_str("96a1f1d5-e37d-4f22-90e0-1817468c9c3e");
  auto l_entity_uuid_id = core_set::get_set().get_uuid();
  insert(l_reg).into<asset_type>().set(c(&asset_type::uuid_id_) = l_uuid, c(&asset_type::name_) = "test")();
  insert(l_reg)
      .into<entity>()
      .set(c(&entity::uuid_id_) = l_entity_uuid_id, c(&entity::name_) = "test", c(&entity::entity_type_id_) = l_uuid)();

  for (auto&& [uuid_id, asset_type] : select(l_reg, &entity::uuid_id_, object_t<asset_type>())
                                          .from<entity>()
                                          .join<asset_type>(&entity::entity_type_id_, &asset_type::uuid_id_)
                                          .where(c(&entity::name_) == "test")
                                          .order_by (&entity::uuid_id_)()) {
    BOOST_TEST_MESSAGE(fmt::format("uuid_id: {}", uuid_id));
    BOOST_TEST_MESSAGE(fmt::format("asset_type name: {}", asset_type.name_));
    BOOST_TEST_CHECK(uuid_id == l_entity_uuid_id);
    BOOST_TEST_CHECK(asset_type.name_ == "test");
  }
  update(l_reg)
      .from<asset_type>()
      .set(c(&asset_type::name_) = "updated_name")
      .where(c(&asset_type::uuid_id_) == l_uuid)();
  for (auto&& [uuid_id, asset_type] : select(l_reg, &entity::uuid_id_, object_t<asset_type>())
                                          .from<entity>()
                                          .join<asset_type>(&entity::entity_type_id_, &asset_type::uuid_id_)
                                          .where(c(&entity::name_) == "test")
                                          .order_by (&entity::uuid_id_)()) {
    BOOST_TEST_MESSAGE(fmt::format("uuid_id: {}", uuid_id));
    BOOST_TEST_MESSAGE(fmt::format("asset_type name: {}", asset_type.name_));
    BOOST_TEST_CHECK(uuid_id == l_entity_uuid_id);
    BOOST_TEST_CHECK(asset_type.name_ == "updated_name");
  }
  std::vector<entity> l_install_entities(50);
  for (int i = 0; i < 50; ++i) {
    l_install_entities[i].uuid_id_        = core_set::get_set().get_uuid();
    l_install_entities[i].name_           = fmt::format("install_entity_{}", i + 1);
    l_install_entities[i].entity_type_id_ = l_uuid;
  }
  auto l_install_1 = insert(l_reg).into<entity>().set_range(l_install_entities);
  l_install_1();
  for (auto& entity : l_install_entities) {
    entity.uuid_id_        = core_set::get_set().get_uuid();
    entity.name_           = fmt::format("updated_{}", entity.uuid_id_);
    entity.entity_type_id_ = l_uuid;
  }
  l_install_1.rebind_range(l_install_entities)();

  for (auto&& [name, uuid_id, asset_type] : select(l_reg, &entity::name_, &entity::uuid_id_, object_t<asset_type>())
                                                .from<entity>()
                                                .join<asset_type>(&entity::entity_type_id_, &asset_type::uuid_id_)
                                                .where(c(&entity::name_).like("updated_%"))
                                                .order_by (&entity::uuid_id_)()) {
    BOOST_TEST_CHECK(name.starts_with("updated_"));
    BOOST_TEST_CHECK(asset_type.name_ == "updated_name");
  }
}

BOOST_AUTO_TEST_SUITE_END()