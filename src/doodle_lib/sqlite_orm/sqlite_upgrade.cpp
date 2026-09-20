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
// 版本号. 生产库当前是 28, 所以只需要一步 28 -> 29.
constexpr std::size_t g_version_28      = 28;
constexpr std::size_t g_current_version = 29;
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
}  // namespace

// 28 -> 29: 把此前被并进 failed 的违规任务回填成 violation.
//
// 背景: 中转站返回的 status=violation 过去被 transfer_station_client::parse_status 一并算作
// failed, 而计费却按模型定价的 charge_on_violation 单独判断 —— 于是违规任务会留下
// 「status=failed 但 completion_tokens 不为 0」的记录. violation 现在是独立状态, 存量数据要按
// data_response 里记录的原始状态回填.
//
// 只改状态, 不动 completion_tokens: 真实库上 8 条失败任务的 token 全是 0 (违规从未落在
// charge_on_violation=true 的模型上), 计费无需修正. 将来若出现需要补扣的记录, 应另行处理.
//
// 条件用 `> 28 就跳过` 而不是 `== 28 才执行`: 版本更旧的库也能被这一步带到最新, 不会出现
// 「版本号写成了最新、迁移却没做」那种不报错的静默失效.
//
// 注意: 27 -> 28 的 schema 重建 (rebuild_all_tables / 外键孤儿清理 / 冗余索引清理) 已随生产库
// 升到 28 而移除. 代价是: 仍停在 27 及更早的库不再补做那一步, 只会执行本步骤.
struct upgrade_1_t : sqlite_upgrade {
  explicit upgrade_1_t() {}
  void upgrade(sqlite_storage& in_data) override {
    using namespace orm;
    namespace sd2 = doodle::seedance2;
    auto l_s      = in_data.create_session();
    // 全新库 (user_version == 0) 已由 upgrade_init_t 建好并标成最新版, 到这里必然跳过
    if (l_s.pragma().user_version() > g_version_28) return;
    backup(l_s);

    // 1. 取出全部失败任务, 逐条按 data_response 里记录的原始状态判断
    auto l_failed = select(l_s)
                        .columns(object<sd2::task>())
                        .from<sd2::task>()
                        .where(c(&sd2::task::status_) == sd2::task_status::failed)()
                        .to_vector();

    // 2. 只有回复里明确写着 violation 的才改判. data_response 为空 / 是字符串 / 没有 status 键的
    //    (提交阶段就失败的任务拿不到完整回复) 一律跳过 —— 判不出来时保持原状, 不凭猜测改状态.
    //    开事务: 要么这批状态全部回填, 要么一条都不改, 避免中途失败留下改了一半的库.
    std::size_t l_fixed{0};
    {
      auto l_guard = l_s.transaction();
      for (const auto& l_task : l_failed) {
        if (!l_task.data_response_.is_object()) continue;
        if (!l_task.data_response_.contains("status")) continue;
        const auto& l_status = l_task.data_response_.at("status");
        if (!l_status.is_string() || l_status.get_ref<const std::string&>() != "violation") continue;

        update(l_s)
            .from<sd2::task>()
            .set(c(&sd2::task::status_) = sd2::task_status::violation)
            .where(c(&sd2::task::uuid_id_) == l_task.uuid_id_)();
        ++l_fixed;
      }
      l_guard.commit();
    }
    SPDLOG_INFO("upgrade 28->29: 检查 {} 条失败任务, 回填 {} 条 violation", l_failed.size(), l_fixed);
    l_s.pragma().user_version(g_current_version);
  }
  ~upgrade_1_t() override = default;
};

std::shared_ptr<sqlite_upgrade> upgrade_init() { return std::make_shared<upgrade_init_t>(); }
std::shared_ptr<sqlite_upgrade> upgrade_1() { return std::make_shared<upgrade_1_t>(); }

}  // namespace doodle::details