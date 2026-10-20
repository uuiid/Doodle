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
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_upgrade.h>

#include "core/core_set.h"
#include "orm/delete.h"
#include "sqlite_orm/orm/alias.h"
#include "sqlite_orm/orm/exception.h"
#include "sqlite_orm/orm/update.h"
#include <chrono>
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
constexpr std::size_t g_version_30      = 30;
constexpr std::size_t g_current_version = 31;
}  // namespace

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

#define DOODLE_ASSET_TYPE(class_name)                            \
  for (const auto& v : class_name::get_all_constant()) {         \
    if (l_session.uuid_to_id<class_name>(v.uuid_id_) == 0) {     \
      auto l_s = std::make_shared<class_name>(v);                \
      orm::insert(l_session).into<class_name>().values (*l_s)(); \
    }                                                            \
  }

    DOODLE_ASSET_TYPE(project_status)
    DOODLE_ASSET_TYPE(assets_helper::database_t)
    DOODLE_ASSET_TYPE(asset_type)
    DOODLE_ASSET_TYPE(task_type)

#undef DOODLE_ASSET_TYPE
  }
};

namespace {
void backup(orm::session& in_data) {
  FSys::path l_file{
      core_set::get_set().get_cache_root("backup") /
      fmt::format("kitsu_{:%Y_%m_%d_%H_%M_%S}.db", chrono::system_clock::now())
  };
  in_data.backup_to(l_file);
}

}  // namespace

struct upgrade_3_t : sqlite_upgrade {
  explicit upgrade_3_t() {}
  void upgrade(sqlite_storage& in_data) override {
    auto l_s = in_data.create_session();
    if (l_s.pragma().user_version() > g_version_30) return;
    backup(l_s);
    l_s.pragma().foreign_keys(false);
    l_s.sync_schema();
    l_s.add_column("seedance2_task_2", "ai_light_session_id", orm::column_type::blob);
    l_s.add_column("seedance2_person_token", "inference_count", orm::column_type::integer);
    l_s.rebuild_table<seedance2::task>();
    l_s.pragma().user_version(g_current_version);
    l_s.pragma().foreign_keys(true);
    SPDLOG_INFO("upgrade 30->31: 已给 ai_studio 补上 apipy_key 列");
  }
  ~upgrade_3_t() override = default;
};

std::shared_ptr<sqlite_upgrade> upgrade_init() { return std::make_shared<upgrade_init_t>(); }
std::shared_ptr<sqlite_upgrade> upgrade_3() { return std::make_shared<upgrade_3_t>(); }

struct upgrade_clear_orphan_task_t : sqlite_upgrade {
  explicit upgrade_clear_orphan_task_t() {}
  void upgrade(sqlite_storage& in_data) override {
    auto l_s = in_data.create_session();
    using namespace orm;
    update(l_s)
        .from<server_task_info>()
        .set(c(&server_task_info::run_computer_id_) = uuid{})
        .where(c(&server_task_info::status_) == server_task_info_status::running)();
    SPDLOG_INFO("upgrade: 启动清理: 已解绑所有 running 任务的 run_computer_id");

    update(l_s)
        .from<seedance2::task>()
        .set(
            c(&seedance2::task::status_)        = seedance2::task_status::failed,
            c(&seedance2::task::data_response_) = nlohmann::json{{"error", "服务端重启, API易 同步请求的结果已丢失"}},
            c(&seedance2::task::ended_at_) =
                chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()}
        )
        .where(c(&seedance2::task::backend_) == seedance2::task_backend::apipy_image && c(&seedance2::task::status_) == seedance2::task_status::running)();
    SPDLOG_INFO("upgrade: 启动清理: 已将残留的 API易 running 任务置为失败");
  }
  ~upgrade_clear_orphan_task_t() override = default;
};

std::shared_ptr<sqlite_upgrade> upgrade_clear_orphan_task() { return std::make_shared<upgrade_clear_orphan_task_t>(); }

}  // namespace doodle::details