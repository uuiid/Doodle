//
// Created by TD on 25-5-15.
//
//

#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/seedance2/subproject.h"
#include "doodle_core/metadata/task.h"
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/seedance2/ai_category.h>
#include <doodle_core/metadata/seedance2/ai_episode.h>
#include <doodle_core/metadata/seedance2/ai_generate_entity.h>
#include <doodle_core/metadata/seedance2/ai_preview_file.h>
#include <doodle_core/metadata/seedance2/task.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_upgrade.h>

#include "core/core_set.h"
#include "sqlite_orm/orm/alias.h"
#include "sqlite_orm/orm/exception.h"
#include "sqlite_orm/orm/update.h"
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace doodle::details {
namespace {
constexpr std::size_t g_current_version = 21;
}

struct upgrade_init_t : sqlite_upgrade {
  explicit upgrade_init_t() {}

  static void full_fts_sync(sqlite_storage& in_data) {
    using namespace orm;
    auto l_s = in_data.create_session();
    try {
      insert(l_s).into<entity_fts>().set(c(any_column<entity_fts>()) = "integrity-check", c(rank()) = 1)();
    } catch (const sqlite_orm_exception& e) {
      SPDLOG_WARN("FTS integrity check failed: {}", e.what());
      insert(l_s).into<entity_fts>().set(c(any_column<entity_fts>()) = "rebuild")();
    }
  }

  void upgrade(sqlite_storage& in_data) override {
    auto l_s = in_data.create_session();
    if (l_s.pragma().user_version() != 0) return;
    l_s.sync_schema();
    l_s.pragma().user_version(g_current_version);
    auto l_session = sqlite_database{in_data.get_strand(), l_s};

#define DOODLE_ASSET_TYPE(class_name)                        \
  for (const auto& v : class_name::get_all_constant()) {     \
    if (l_session.uuid_to_id<class_name>(v.uuid_id_) == 0) { \
      auto l_s = std::make_shared<class_name>(v);            \
      l_session.install_unsafe<class_name>(l_s);             \
    }                                                        \
  }

    DOODLE_ASSET_TYPE(project_status)
    DOODLE_ASSET_TYPE(assets_helper::database_t)
    DOODLE_ASSET_TYPE(asset_type)
    DOODLE_ASSET_TYPE(task_type)

#undef DOODLE_ASSET_TYPE
  }
};  // namespace doodle::details

namespace {
void backup(orm::session& in_data) {
  FSys::path l_file{
      core_set::get_set().get_cache_root("backup") /
      fmt::format("kitsu_{:%Y_%m_%d_%H_%M_%S}.db", chrono::system_clock::now())
  };
  in_data.backup_to(l_file);
}
}  // namespace

struct upgrade_2_t : sqlite_upgrade {
  explicit upgrade_2_t() {}
  void upgrade(sqlite_storage& in_data) override {
    // auto l_s = in_data.create_session();
    // if (l_s.pragma().user_version() == 19) {
    // }
    // l_s.pragma().user_version(g_current_version);
  }
  ~upgrade_2_t() override = default;
};

struct upgrade_3_t : sqlite_upgrade {
  explicit upgrade_3_t() {}
  void upgrade(sqlite_storage& in_data) override {
    auto l_s = in_data.create_session();
    if (l_s.pragma().user_version() != 20) return;
    // 为 seedance2_task_2 添加 retry_count 列
    l_s.exec(R"(ALTER TABLE "seedance2_task_2" ADD COLUMN "retry_count" INTEGER NOT NULL DEFAULT 0;)");
    l_s.pragma().user_version(g_current_version);
  }
  ~upgrade_3_t() override = default;
};

std::shared_ptr<sqlite_upgrade> upgrade_init() { return std::make_shared<upgrade_init_t>(); }
std::shared_ptr<sqlite_upgrade> upgrade_1() { return std::make_shared<upgrade_2_t>(); }
std::shared_ptr<sqlite_upgrade> upgrade_2() { return std::make_shared<upgrade_3_t>(); }

}  // namespace doodle::details