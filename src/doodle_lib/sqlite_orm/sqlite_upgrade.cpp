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
    if (in_data.pragma().user_version() == 6) {
      auto l_s = std::make_shared<asset_type>(asset_type{
          .uuid_id_     = asset_type::get_half_ai_id(),
          .name_        = "半AI",
          .short_name_  = "半AI",
      });
      in_data.install_unsafe<asset_type>(l_s);
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