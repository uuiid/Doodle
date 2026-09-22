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
// 版本号. 生产库当前是 29, 所以只需要一步 29 -> 30.
constexpr std::size_t g_version_29      = 29;
constexpr std::size_t g_current_version = 30;
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
};  // namespace doodle::details

namespace {
void backup(orm::session& in_data) {
  FSys::path l_file{
      core_set::get_set().get_cache_root("backup") /
      fmt::format("kitsu_{:%Y_%m_%d_%H_%M_%S}.db", chrono::system_clock::now())
  };
  in_data.backup_to(l_file);
}

// 表上是否已有某列.
// ALTER TABLE ADD COLUMN 没有 IF NOT EXISTS, 重复执行会报 "duplicate column name".
// 版本号与 schema 可能被中途打断而对不上 (例如 ALTER 成功但 user_version 没写下去),
// 所以执行前必须先查一次, 否则下次启动就会直接失败.
bool has_column(orm::session& in_session, const std::string& in_table, const std::string& in_column) {
  orm::sqlite_stmt l_stmt{
      in_session, fmt::format("SELECT 1 FROM pragma_table_info('{}') WHERE name = '{}';", in_table, in_column)
  };
  return l_stmt.step_not_throw() == SQLITE_ROW;
}

// 表是否存在. 老库可能还没有某张表, 直接对不存在的表执行语句会让升级整个失败,
// 而"表不存在"本身就意味着这一步没有要清理的数据.
bool has_table(orm::session& in_session, const std::string& in_table) {
  orm::sqlite_stmt l_stmt{
      in_session, fmt::format("SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = '{}';", in_table)
  };
  return l_stmt.step_not_throw() == SQLITE_ROW;
}
}  // namespace

// 29 -> 30: entity_asset_extend 增加「是否包含特写」te_xie (默认 false).
//
// 用 ALTER TABLE 补列, 不用 rebuild_table: 重建的列拷贝清单来自 ORM 声明 (session::rebuild_table),
// 会把新列也写进 SELECT, 而旧表里没有这一列, 直接报 no such column.
// 带 DEFAULT 0: 存量行在库层面就落成 0/false, 不依赖 ORM 读到 NULL 时的兜底行为.
//
// 条件用 `> 29 就跳过` 而不是 `== 29 才执行`: 版本更旧的库也能被这一步带到最新, 不会出现
// 「版本号写成了最新、迁移却没做」那种不报错的静默失效.
//
// 注意: 27 -> 28 的 schema 重建 (rebuild_all_tables / 外键孤儿清理 / 冗余索引清理) 与
// 28 -> 29 的 violation 回填都已随生产库升到 29 而移除 —— 生产库已经在 29 上, 那两步不会再
// 执行; 仍停在 28 及更早的库只会执行本步骤 (代价是拿不到那两步的修复).
//
// 表不存在时本步骤会直接失败并中止升级, 这是刻意的: v29 生产库必然有 entity_asset_extend_2
// (资产写入路径一直在用它), 缺表说明库已损坏, 静默跳过只会让 schema 与 ORM 声明长期不一致.
struct upgrade_2_t : sqlite_upgrade {
  explicit upgrade_2_t() {}
  void upgrade(sqlite_storage& in_data) override {
    auto l_s = in_data.create_session();
    // 全新库 (user_version == 0) 已由 upgrade_init_t 建好 (含 te_xie) 并标成最新版, 到这里必然跳过
    if (l_s.pragma().user_version() > g_version_29) return;
    backup(l_s);

    if (has_column(l_s, "entity_asset_extend_2", "te_xie")) {
      SPDLOG_INFO("upgrade 29->30: entity_asset_extend_2.te_xie 已存在, 跳过 ALTER");
    } else {
      l_s.add_column("entity_asset_extend_2", "te_xie", "INTEGER", "0");
      SPDLOG_INFO("upgrade 29->30: 已添加 entity_asset_extend_2.te_xie (DEFAULT 0, 存量行读出 false)");
    }
    l_s.pragma().user_version(g_current_version);
  }
  ~upgrade_2_t() override = default;
};

std::shared_ptr<sqlite_upgrade> upgrade_init() { return std::make_shared<upgrade_init_t>(); }
std::shared_ptr<sqlite_upgrade> upgrade_2() { return std::make_shared<upgrade_2_t>(); }

// 启动清理: 解绑所有 running 任务的 run_computer_id.
//
// 服务端重启(或工作机崩溃)时断开处理没机会执行, 库里会残留 status_ = running 且
// run_computer_id 指向某台机器的任务. 派发判据里包含「库里没有绑在这台机器上的 running
// 任务」(computers_assign_task::run_next_task), 所以这条残留绑定会让那台机器被永久挡住 ——
// 即使它已经重启并上报 online, 也一个任务都拿不到.
//
// 只清绑定, 状态保留 running 供人工确认: 不改成 submitted, 避免工作机其实还在跑同一个
// 任务时被重复执行.
//
// 刻意**不看也不写 user_version**: 这不是 schema 迁移, 而是每次启动都要重新执行的清理,
// 版本号门控会让"上次启动留下的残留绑定"再也清不掉.
struct upgrade_clear_orphan_task_t : sqlite_upgrade {
  explicit upgrade_clear_orphan_task_t() {}
  void upgrade(sqlite_storage& in_data) override {
    auto l_s = in_data.create_session();
    using namespace orm;
    if (!has_table(l_s, "server_task_info_tab")) {
      SPDLOG_INFO("upgrade: 启动清理: 库里没有 server_task_info_tab, 跳过 running 任务解绑");
      return;
    }
    update(l_s)
        .from<server_task_info>()
        .set(c(&server_task_info::run_computer_id_) = uuid{})
        .where(c(&server_task_info::status_) == server_task_info_status::running)();
    SPDLOG_INFO("upgrade: 启动清理: 已解绑所有 running 任务的 run_computer_id");
  }
  ~upgrade_clear_orphan_task_t() override = default;
};

std::shared_ptr<sqlite_upgrade> upgrade_clear_orphan_task() {
  return std::make_shared<upgrade_clear_orphan_task_t>();
}

}  // namespace doodle::details