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
// 版本号. 生产库当前是 30, 30 之后只有**一步**: 30 -> 31 (建 AI 灯光六张表).
//
// 规则: **只有最后一步和 upgrade_init_t 写 g_current_version, 非最后的中间步骤写自己的目标版本常量。**
// 判断某一步该不该写 g_current_version 只有一个依据: 它是不是最后一步。
//
// 违反这条规则的错误是无声的: 早期 upgrade_2_t (29 -> 30) 写的就是 g_current_version, 那时两者恰好都等于
// 30 所以没暴露问题; 一旦 g_current_version 被提到更高而某个非最后步骤仍写它, 更旧的库会在同一次启动里被
// stamp 过头, 那个后续步骤的 `> N 就跳过` 判定随即成立、**静默跳过**, 而版本号看起来完全正确。
// 现在版本化步骤只剩 30 -> 31 一步, 所以 upgrade_3_t 写 g_current_version 是正确的;
// **将来在它之前或之后再插步骤时, 必须回头检查这一行。**
//
// 29 -> 30 (upgrade_2_t: 给 entity_asset_extend_2 补 te_xie 列) 已随该步骤进入生产而**移除**。
// 代价是刻意的: 升级链现在只认「30 及以上」。仍停在 30 之前的库 (例如开发机上的旧备份) 只会执行
// 30 -> 31 —— 它能拿到 AI 灯光六张表, 但**拿不到 te_xie 列**, 于是 ORM 读写这张表时会报
// "no such column"。这类库不受支持, 需要重建或手工补列。同一取舍的先例: 27 -> 28 与 28 -> 29
// 两步也是随生产库推进而移除的 (见 docs/ue_ai_lighting_plan.md §15.3)。
//
// 另外, 删掉 ai_light_session_state 的 accepted / rejected / failed 三个枚举量**不需要数据迁移**:
// 停在 30 的库里根本没有 ai_light_* 表 (唯一的建表者就是 30 -> 31 这一步), 六张表出生即空,
// 所以任何生产库里都不可能存在那三个字符串; 删枚举量也不是 schema 变更 (state 是 TEXT 列)。
// 详见 docs/ue_ai_lighting_plan.md §15.10。
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

// 30 -> 31: 建 AI 灯光六张表 (docs/ue_ai_lighting_plan.md §10.2).
//
// 这一步没有 ALTER, 因为全部改动都是**新表**:
//   ai_light_session / ai_light_reference / ai_light_spec /
//   ai_light_iteration / ai_light_candidate / ai_light_preset
//
// sync_schema() 的语义正好够用 (见 orm/session.cpp:168-217):
//   * 表已存在 → 直接 continue, **不会给已有表加列、改列**, 所以对存量 schema 零风险;
//   * 表不存在 → 建表;
//   * 索引/触发器逐个按名字查, 缺哪个建哪个。
// 也就是说它只能新增、不能修改, 这正是本步骤需要的。
//
// 之所以不用 create_table(单个 type_index): 它只建表、不建索引
// (session.cpp:326-337), 那六张表的唯一索引就得手写 SQL。
// （定稿唯一性用的是 accepted_session_id 上的可空唯一索引，不是 partial index ——
//  本库生成 partial index 时会把条件写成绑定参数，SQLite 拒绝，见 sqlite_database.h。）
//
// 这是 30 之后的唯一一步, 所以没有、也不需要任何数据改写: 六张表在这里出生即空,
// 本轮取消会话终态 (accepted / rejected / failed) 只是删枚举量、不涉及 schema 变更。
// 详见文件顶部版本号注释与 docs/ue_ai_lighting_plan.md §15.10。
//
// 条件用 `> 30 就跳过`: 版本更旧的库也一并被带到最新 (但它会缺 30 之前那些步骤补的列, 见顶部注释)。
// 全新库 (user_version == 0) 已由 upgrade_init_t 的 sync_schema 建好并标成最新版, 到这里必然跳过。
struct upgrade_3_t : sqlite_upgrade {
  explicit upgrade_3_t() {}
  void upgrade(sqlite_storage& in_data) override {
    auto l_s = in_data.create_session();
    if (l_s.pragma().user_version() > g_version_30) return;
    backup(l_s);
    l_s.pragma().foreign_keys(false);
    l_s.sync_schema();
    // 这两列都必须存在于旧表里, 且必须都在 rebuild_table **之前**补上. 原因是 rebuild_table 的
    // 列拷贝清单来自 **ORM 声明**而不是库里的实际列 (见 orm/session.cpp:236-242 与
    // docs/3-sqlite_schema_cleanup_changelog.md 7.3), 它会生成
    // `INSERT INTO ..._backup ("...", "result_text", ...) SELECT ... "result_text" FROM ...`.
    // 一旦 result_text 在旧表里不存在, SQLite 会把双引号标识符**降级成字符串字面量** (DQS,
    // 经 sqlite3_config(SQLITE_CONFIG_LOG) 回调报 warning 28 `double-quoted string literal: "result_text"`),
    // 而**不是**报错: 重建看起来成功, 14k 行却全被写成字面量 'result_text', 静默丢数据.
    // 所以这里刻意用 add_column 显式补列, 不能依赖 sync_schema —— 它对已存在的表直接 continue,
    // **不会加列** (见本文件 120-124 行注释).
    // 不需要先查列是否已存在: 上面的 `> 30 就跳过` 保证本步骤对一张库只跑一次.
    l_s.add_column("seedance2_task_2", "ai_light_iteration_id", orm::column_type::blob);
    l_s.add_column("seedance2_task_2", "result_text", orm::column_type::text);
    l_s.rebuild_table<seedance2::task>();
    // 本步骤是 30 之后的**唯一**一步, 所以它写 g_current_version (= 31)。
    // 将来若在它后面再插步骤, 这一行必须改成写自己的目标版本常量, 否则下一步会被静默跳过
    // —— 见文件顶部版本号注释。
    l_s.pragma().user_version(g_current_version);
    l_s.pragma().foreign_keys(true);
    SPDLOG_INFO(
        "upgrade 30->31: 已创建 AI 灯光六张表 (ai_light_session / _reference / _spec / _iteration / _candidate / "
        "_preset)"
    );
  }
  ~upgrade_3_t() override = default;
};

std::shared_ptr<sqlite_upgrade> upgrade_init() { return std::make_shared<upgrade_init_t>(); }
std::shared_ptr<sqlite_upgrade> upgrade_3() { return std::make_shared<upgrade_3_t>(); }

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
    if (!l_s.has_reg_table<server_task_info>()) {
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

std::shared_ptr<sqlite_upgrade> upgrade_clear_orphan_task() { return std::make_shared<upgrade_clear_orphan_task_t>(); }

}  // namespace doodle::details