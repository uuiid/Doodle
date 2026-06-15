//
// Created by TD on 25-5-15.
//
//

#include "doodle_core/metadata/entity.h"
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/seedance2/task.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_upgrade.h>

#include <boost/hana/ext/std/tuple.hpp>

#include "sqlite_orm/orm/bind_value.h"
#include "sqlite_orm/orm/select.h"
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include <tuple>

namespace doodle::details {
namespace {
constexpr std::size_t g_current_version = 7;
}

struct upgrade_init_t : sqlite_upgrade {
  // 755c9edd-9481-4145-ab43-21491bdf2739
  static constexpr uuid g_open_id{
      {0x75, 0x5c, 0x9e, 0xdd, 0x94, 0x81, 0x41, 0x45, 0xab, 0x43, 0x21, 0x49, 0x1b, 0xdf, 0x27, 0x39}
  };
  // 0196eb9d5dc0727d8a751b05dea8494d
  static constexpr uuid g_lable_id{
      {0x01, 0x96, 0xeb, 0x9d, 0x5d, 0xc0, 0x72, 0x7d, 0x8a, 0x75, 0x1b, 0x05, 0xde, 0xa8, 0x49, 0x4d}
  };
  // 5159f210-7ec8-40e3-b8c9-2a06d0b4b116
  static constexpr uuid g_closed_id{
      {0x51, 0x59, 0xf2, 0x10, 0x7e, 0xc8, 0x40, 0xe3, 0xb8, 0xc9, 0x2a, 0x06, 0xd0, 0xb4, 0xb1, 0x16}
  };
  explicit upgrade_init_t(const FSys::path& in_path) {}

  static void full_fts_sync(sqlite_database& in_data) {
    using namespace orm;
    insert(in_data).into<entity_fts>().set(c(any_column<entity_fts>()) = "rebuild")();
  }

  void upgrade(sqlite_database& in_data) override {
    if (in_data.pragma().user_version() != 0) return;
    in_data.sync_schema();
    in_data.pragma().user_version(g_current_version);

#define DOODLE_ASSET_TYPE(class_name)                      \
  for (const auto& v : class_name::get_all_constant()) {   \
    if (in_data.uuid_to_id<class_name>(v.uuid_id_) == 0) { \
      auto l_s = std::make_shared<class_name>(v);          \
      in_data.install_unsafe<class_name>(l_s);             \
    }                                                      \
  }

    DOODLE_ASSET_TYPE(project_status)
    DOODLE_ASSET_TYPE(assets_helper::database_t)
    DOODLE_ASSET_TYPE(asset_type)
    DOODLE_ASSET_TYPE(task_type)

#undef DOODLE_ASSET_TYPE
  }
};  // namespace doodle::details

struct upgrade_2_t : sqlite_upgrade {
  explicit upgrade_2_t(const FSys::path& in_path) {}
  void upgrade(sqlite_database& in_data) override {
    if (in_data.pragma().user_version() == 2) {
      upgrade_init_t::full_fts_sync(in_data);
      in_data.pragma().user_version(g_current_version);
    }

    in_data.pragma().user_version(g_current_version);
  }
  ~upgrade_2_t() override = default;
};

std::shared_ptr<sqlite_upgrade> upgrade_init(const FSys::path& in_db_path) {
  return std::make_shared<upgrade_init_t>(in_db_path);
}
std::shared_ptr<sqlite_upgrade> upgrade_1(const FSys::path& in_db_path) {
  return std::make_shared<upgrade_2_t>(in_db_path);
}

}  // namespace doodle::details