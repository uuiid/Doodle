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
#include <boost/scope/scope_exit.hpp>
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
// 版本号. 生产库当前是 27, 所以只需要一步 27 -> 28.
constexpr std::size_t g_version_27      = 27;
constexpr std::size_t g_current_version = 28;
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

// 27 -> 28: 把 regs_all() 里累积的 schema 改动一次性落地到已有库.
//
// 外键的目标和动作都写在 CREATE TABLE 里, 光改 C++ 声明对已有表没有任何影响, 必须重建表.
// 本次落地的内容:
//   * comment.object_id 的外键目标 entity -> task (原先对全部数据都不成立)
//   * 字典表外键 cascade -> no_action (task / entity / playlist / comment / status_automation /
//     project), 避免"删一个字典项"静默清空大批业务数据
//   * 列声明为 NOT NULL 却配 ON DELETE SET NULL 的两处 (comment.person_id /
//     seedance2_subproject.created_user_id) -> no_action, 否则删 person 必然撞 NOT NULL 约束
//   * 补齐此前是裸列的外键: comment.object_id / assets_tab.parent_uuid /
//     work_xlsx_task_info_tab.project_id
//   * 两处可空的可选归属补 set_null 外键: task.last_preview_file_id -> preview_file,
//     work_xlsx_task_info_tab.kitsu_task_ref_id -> task
//   * 补 3 处唯一索引: preview_file 取消单列 unique, project_asset_type_link /
//     project_person_link / project_status_automation_link / project_preview_background_file_link
//   * 不再生成与被引用列 UNIQUE 自动索引重复的冗余索引
//
// 注意: 三张废弃表 (metadata_descriptor / metadata_descriptor_department_link / ai_image_metadata)
// 的删除动作**已从本步骤移除**. 生产库升级时已一次性删掉它们, 而它们不在 regs_all() 里, ORM
// 完全不会碰. 代价是: 若还有停留在 v27 的库, 升级后会保留这三张空表 (不影响任何功能).
//
// 条件用 `> 27 就跳过` 而不是 `== 27 才执行`: 本步骤的内容是此前所有升级动作的**并集**, 所以
// 任何比当前版本旧的库都能被它一次带到最新. 若写成 `== 27`, 版本停在 26 的库既不满足条件,
// 也不会被写入新版本号, 于是永远留在旧 schema 上 —— 正是那种不报错、但迁移根本没做的失效方式.
struct upgrade_1_t : sqlite_upgrade {
  explicit upgrade_1_t() {}
  void upgrade(sqlite_storage& in_data) override {
    using namespace orm;
    auto l_s = in_data.create_session();
    // 全新库 (user_version == 0) 已由 upgrade_init_t 建好并标成最新版, 到这里必然跳过
    if (l_s.pragma().user_version() > g_version_27) return;
    backup(l_s);

    // 1. 重建全部已注册的表, 让新的 schema 生效
    const auto l_rebuilt = in_data.rebuild_all_tables(l_s);

    // 2. 重建会**新暴露**孤儿子行: comment.object_id 指向已删除任务的那些行, 在旧外键下
    //    (指向 entity) 就是违规的, 换成指向 task 之后依然违规. 必须在重建之后清掉,
    //    否则这些行此后任何 UPDATE 都会因外键约束失败.
    //    PRAGMA foreign_keys 在事务内是 no-op, 必须在 BEGIN 之前关闭; 关闭后 ON DELETE
    //    CASCADE / SET NULL 不触发, 被"孤立"的子行交给下一轮 fixpoint 处理.
    //    连接级默认值已由 storage::register_custom_extension 统一设为 ON, 所以这里必须**显式**
    //    关闭, 不能依赖"默认就是 OFF"; 关闭状态也不能泄漏回连接池, 用 guard 恢复原值.
    {
      const auto l_fk_was_on = l_s.pragma().foreign_keys();
      l_s.pragma().foreign_keys(false);
      boost::scope::scope_exit l_fk_guard([&l_s, l_fk_was_on]() { l_s.pragma().foreign_keys(l_fk_was_on); });
      auto l_guard = l_s.transaction();
      // 2a. 清理外键违规. fix_foreign_key_violations 内部每一轮都会先置空悬空的可选归属引用,
      //     再删孤儿行 —— 顺序反了会把"唯一问题是悬空可选引用"的有效行整行删掉
      //     (真实库上是 142 个任务 + 108 条工时记录).
      auto l_deleted = in_data.fix_foreign_key_violations(l_s);
      l_guard.commit();
      SPDLOG_INFO("upgrade 27->28: 重建 {} 张表, 清理 {} 行外键孤儿", l_rebuilt, l_deleted);
    }

    // 3. 清理**未注册的遗留表**上的冗余索引: 它们不在 regs_all() 里, 重建碰不到
    in_data.drop_redundant_indexes(l_s);

    l_s.vacuum();
    l_s.pragma().user_version(g_current_version);
  }
  ~upgrade_1_t() override = default;
};

std::shared_ptr<sqlite_upgrade> upgrade_init() { return std::make_shared<upgrade_init_t>(); }
std::shared_ptr<sqlite_upgrade> upgrade_1() { return std::make_shared<upgrade_1_t>(); }

}  // namespace doodle::details