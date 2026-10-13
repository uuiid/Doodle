//
// Created by TD on 24-9-12.
//

#pragma once

#include "doodle_core/metadata/entity.h"
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/scan_data_t.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>

#include <boost/asio/awaitable.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include <doodle_lib/sqlite_orm/orm/delete.h>
#include <doodle_lib/sqlite_orm/orm/session.h>
#include <optional>
#include <range/v3/view/unique.hpp>
#include <stdexcept>
#include <vector>

namespace doodle {
struct preview_file;
struct preview_files_for_entity_t;
enum class computer_status;
struct status_automation;
enum class server_task_info_type;
struct asset_type;
class server_task_info;
struct todo_t;
struct get_comments_t;
struct task_status;
struct assets_and_tasks_t;
struct entities_and_tasks_t;
struct department;
struct comment;
struct task;
struct entity_link;
struct project_task_status_link;
struct entity_asset_extend;
struct playlist_shot;
struct entity_shot_extend;
struct task_type_asset_type_link;
struct working_file;
namespace attendance_helper {
struct database_t;
}
namespace assets_file_helper {
struct database_t;
struct link_parent_t;
}  // namespace assets_file_helper
namespace work_xlsx_task_info_helper {
struct database_t;
}
namespace ai_light {
struct ai_light_session;
struct ai_light_reference;
struct ai_light_iteration;
struct ai_light_candidate;
struct ai_light_preset;
enum class ai_light_session_state;
}  // namespace ai_light
}  // namespace doodle
namespace doodle {

class DOODLELIB_API sqlite_storage : public orm::storage {
  void open_(FSys::path in_path, std::int32_t in_flags) override;
  void register_custom_extension(sqlite3* in_sqlite) override;
  using strand_type = boost::asio::strand<boost::asio::io_context::executor_type>;
  strand_type strand_{boost::asio::make_strand(g_io_context())};

 public:
  void regs_all();
  // 升级
  void upgrade();
  // 反复执行 PRAGMA foreign_key_check 并删除违规行, 直到没有违规为止.
  // **每一轮都会先调用 null_dangling_optional_references 把可空可选归属列上的悬空引用置空**,
  // 再删剩下的孤儿行. 这个顺序是必须的: 删父行会在同一轮里造出新的悬空引用, 若只删不置空,
  // 下一轮会把"唯一问题是悬空可选引用"的有效行整行删掉. 两者一起构成"按外键语义修复违规".
  // 不管理事务: 由调用方决定事务边界, 且需在 BEGIN 之前 PRAGMA foreign_keys = OFF.
  // @param in_max_rounds 最大轮数
  // @param in_chunk_size 单条 DELETE 的最大 rowid 数 (绑定参数上限保护)
  // @return 累计删除的行数 (置空的行数记在日志里)
  std::size_t fix_foreign_key_violations(
      orm::session& in_session, std::size_t in_max_rounds = 50, std::size_t in_chunk_size = 500
  );
  // 重建所有已注册的实体表 (逐表调用 session::rebuild_table).
  // 跳过: 伪表 (sqlite_master / pragma_foreign_key_check)、FTS5 虚拟表、以及库中尚未创建的表.
  // 期间会临时关闭外键约束 (重建会 DROP TABLE, 开着外键会触发级联或约束错误).
  // rebuild_table 在复制数据前会先删除该表自己的触发器并事后恢复, 因此 entity 这类带 FTS 同步
  // 触发器的表不会把索引写重复.
  // @return 实际重建的表数量
  std::size_t rebuild_all_tables(orm::session& in_session);
  // 删除"纯冗余索引": 某个显式索引的列集合与同表上 sqlite_autoindex_* (由 UNIQUE/PK 自动生成)
  // 完全相同. 这种索引对查询毫无帮助, 只会在每次写该表时被重复维护并额外占空间.
  // 对已注册的表, rebuild_all_tables 已经不会再生成它们; 本函数用于清理**未注册的遗留表** ——
  // 那些表不在 regs_all() 里, 重建碰不到.
  // @return 实际删除的索引数量
  std::size_t drop_redundant_indexes(orm::session& in_session);
  // 把**可空的可选归属列**上已经悬空的引用置空.
  // 与 fix_foreign_key_violations 的区别很关键: 后者删的是"孤儿子行"(行本身就不该存在),
  // 而这里的行本身是有效的, 只是指向了一个已不存在的对象 —— 直接删行会丢业务数据
  // (真实库里有 79 个任务的 last_preview_file_id 悬空, 按孤儿删掉就是删掉 79 个任务).
  // 通常不需要单独调用: fix_foreign_key_violations 每一轮都会先调用它再删行.
  // @return 被置空的行数
  std::size_t null_dangling_optional_references(orm::session& in_session);
  strand_type get_strand() { return strand_; }
};

class DOODLELIB_API sqlite_database {
  using strand_type = boost::asio::strand<boost::asio::io_context::executor_type>;
  strand_type strand_;
  orm::session session_{};

 public:
  std::vector<uuid> get_temporal_type_ids();

  explicit sqlite_database(strand_type in_strand, orm::session in_session)
      : strand_(std::move(in_strand)), session_(std::move(in_session)) {}
  ~sqlite_database() = default;
  // operator orm::session() { return session_; }
  operator orm::session() const { return session_; }
  orm::session get_session() const { return session_; }
  /// 备份数据库
  boost::asio::awaitable<void> backup(FSys::path in_path);

  template <typename T>
  std::vector<T> get_all() {
    using namespace orm;
    return select(*this).columns(object<T>()).template from<T>()().to_vector();
  }

  template <typename T>
  std::int64_t uuid_to_id(uuid in_uuid) {
    using namespace orm;
    return select(*this).columns(&T::id_).template from<T>().where(c(&T::uuid_id_) == in_uuid)().to_optional().value_or(
        0
    );
  }

  template <typename T>
  uuid id_to_uuid(std::int64_t in_id) {
    using namespace orm;
    return select(*this)
        .columns(&T::uuid_id_)
        .template from<T>()
        .where(c(&T::id_) == in_id)()
        .to_optional()
        .value_or(uuid{});
  }

  template <typename T>
  T get_by_uuid(uuid in_uuid) {
    using namespace orm;
    return select(*this).columns(object<T>()).template from<T>().where(c(&T::uuid_id_) == in_uuid)().to_single();
  }
#define DOODLE_TO_SQLITE_THREAD()                                     \
  DOODLE_CHICK(!core_set::get_set().read_only_mode_, "只读不可保存"); \
  auto this_executor = co_await boost::asio::this_coro::executor;     \
  co_await boost::asio::dispatch(boost::asio::bind_executor(strand_, boost::asio::use_awaitable));

  boost::asio::awaitable<void> run_sql(orm::sql_modify_statement_vector_t in_sqls);

  std::vector<attendance_helper::database_t> get_attendance(
      const uuid& in_person_id, const chrono::local_days& in_data
  );
  std::vector<attendance_helper::database_t> get_attendance(
      const uuid& in_person_id, const std::vector<chrono::local_days>& in_data
  );
  std::vector<work_xlsx_task_info_helper::database_t> get_work_xlsx_task_info(
      const uuid& in_person_id, const chrono::local_days& in_data
  );

  person get_person_for_email(const std::string& in_email);
  /// 获取用户所在的团队对应的项目
  std::vector<project> get_person_projects(const person& in_user);

  std::optional<project_task_type_link> get_project_task_type_link(
      const uuid& in_project_id, const uuid& in_task_type_id
  );
  std::optional<project_task_status_link> get_project_task_status_link(
      const uuid& in_project_id, const uuid& in_task_status_uuid
  );
  std::optional<project_asset_type_link> get_project_asset_type_link(
      const uuid& in_project_id, const uuid& in_asset_type_uuid
  );
  std::vector<person> get_project_persons(const uuid& in_project_uuid);
  // 查询人员是否在项目团队中
  bool is_person_in_project(const person& in_person, const uuid& in_project_id);
  bool is_person_in_project(const uuid& in_person, const uuid& in_project_id);
  // 查询对应的task是否存在
  bool is_task_exist(const uuid& in_entity_id, const uuid& in_task_type_id);

  task_status get_task_status_by_name(const std::string& in_name);
  asset_type get_entity_type_by_name(const std::string& in_name);

  // 获取用户的订阅
  std::set<uuid> get_person_subscriptions(
      const person& in_person, const uuid& in_project_id, const uuid& in_asset_type_uuid
  ) {
    return get_person_subscriptions(in_person.uuid_id_, in_project_id, {in_asset_type_uuid});
  }
  std::set<uuid> get_person_subscriptions(
      const uuid& in_person_id, const uuid& in_project_id, const std::vector<uuid>& in_asset_type_uuid
  );

  std::set<uuid> get_notification_recipients(const task& in_task);
  std::set<uuid> get_mentioned_people(const uuid& project_id, const comment& in_comment_id);
  std::vector<status_automation> get_project_status_automations(const uuid& in_project_uuid);
  /// 返回一个允许将任务类型id与优先级匹配的字典。
  std::map<uuid, std::int32_t> get_task_type_priority_map(const uuid& in_project, const std::string& in_for_entity);
  /// 返回所属实体,类别 的任务
  std::optional<task> get_tasks_for_entity_and_task_type(const uuid& in_entity_id, const uuid& in_task_type_id);
  /// 在模型库中, 是否有和资产类别关联的模型
  bool has_assets_tree_assets_link(const uuid& in_label_uuid);
  /// 是否存在 label_assets_link
  bool has_assets_tree_assets_link(const uuid& in_label_uuid, const uuid& in_asset_uuid);
  /// 模型库中,是否有资产类别的子类别
  bool has_assets_tree_child(const uuid& in_label_uuid);
  /// 获取资产类别和模型的连接
  assets_file_helper::link_parent_t get_assets_tree_assets_link(const uuid& in_label_uuid, const uuid& in_asset_uuid);
  ///
  std::map<uuid, std::vector<preview_files_for_entity_t>> get_preview_files_for_entity(const uuid& in_entity_id);
  /// 获取评论对应的预览图
  std::optional<preview_file> get_preview_file_for_comment(const uuid& in_comment_id);
  /// 是否将任务分配给了用户
  bool is_task_assigned_to_person(const uuid& in_task, const uuid& in_person);

  /// 获取任务的下一个预览版本号
  std::int64_t get_next_preview_revision(const uuid& in_task_id);
  /// 当前评论是否有预览图
  bool has_preview_file(const uuid& in_comment);
  /// 获取评论的预览图位置(一条评论中可能有多个预览图, 图片序列)
  std::int64_t get_next_position(const uuid& in_task_id, const std::int64_t& in_revision);
  /// 获取评论预览图对应的版本号
  std::int64_t get_preview_revision(const uuid& in_comment);
  /// 获取task中的最后一条评论
  std::optional<comment> get_last_comment(const uuid& in_task_id);
  /// 获取资产对应的 task
  std::vector<task> get_tasks_for_entity(const uuid& in_asset_id);

  std::optional<entity_link> get_entity_link(const uuid& in_entity_in_id, const uuid& in_asset_id);
  /// 获取任务额外数据
  std::optional<entity_asset_extend_value> get_entity_asset_extend(const uuid& in_entity_id);
  /// 获取镜头扩展数据
  std::optional<entity_shot_extend> get_entity_shot_extend(const uuid& in_entity_id);
  /// 获取播放序列对应的实体
  std::vector<playlist_shot> get_playlist_shot_entity(const uuid& in_playlist_id);

  std::optional<task_type_asset_type_link> get_task_type_asset_type_link(
      const uuid& in_task_type_id, const uuid& in_asset_type_id
  );
  uuid get_project_status_open();
  uuid get_project_status_closed();

  // 获取 项目中实体数量
  std::size_t get_project_entity_count(const uuid& in_project_id);

  // 是给外包授权的实体
  bool is_entity_outsourced(const uuid& in_entity_id, const uuid& in_studio_id, const uuid& in_parent_id = uuid{});
  // 按照计算机id 获取工作
  std::vector<server_task_info> get_server_tasks_by_submitted();
  // 当前仍处于 running 的任务所绑定的计算机 id 集合, 用于判断某台机器是否还有未完成的工作
  std::set<uuid> get_running_task_computer_ids();
  // 获取镜头任务对应的 场景资产的扩展数据 如果没有, 抛出异常, 大于一个, 抛出异常
  entity_asset_extend_value get_entity_shot_extend_by_task(const uuid& in_shot_id);

  // ---------------------------------------------------------------------------
  // AI 灯光（docs/ue_ai_lighting_plan.md）
  //
  // 这里刻意只放**查询**：会话/轮次的状态推进由轮次编排负责，
  // 不往 sqlite_database 里塞业务写操作。
  //
  // 几个「当前」是靠查询得出而不是靠冗余指针列的，对应关系：
  //   本轮起点      = 上一轮 is_selected 的候选     → get_ai_light_selected_candidate
  //   定稿候选      = is_accepted 的候选            → get_ai_light_accepted_candidate
  //
  // 曾经还有「当前 Spec = 会话内 revision 最大的一行 → get_current_ai_light_spec」，
  // 已随 ai_light_spec 表一起删除。
  // ---------------------------------------------------------------------------

  /// 镜头下的全部会话，新的在前。Shot 页入口。
  std::vector<ai_light::ai_light_session> get_ai_light_sessions_by_entity(const uuid& in_entity_id);
  /// 项目下的全部会话，新的在前
  std::vector<ai_light::ai_light_session> get_ai_light_sessions_by_project(const uuid& in_project_id);
  /// 按状态查会话（前端「待办」列表用它捞 awaiting_judgement）。
  /// 「已过期」不是一个状态、查不到它 —— 过期由 `ai_light_session::is_judgement_overdue()`
  /// 在 C++ 侧实时算，读模型带出 is_judgement_overdue_。
  std::vector<ai_light::ai_light_session> get_ai_light_sessions_by_state(
      ai_light::ai_light_session_state in_state
  );

  /// 会话的参考图，按 created_at 升序（= 上传先后顺序）
  std::vector<ai_light::ai_light_reference> get_ai_light_references(const uuid& in_session_id);

  /// 会话的全部轮次，iteration 升序
  std::vector<ai_light::ai_light_iteration> get_ai_light_iterations(const uuid& in_session_id);
  /// 指定轮次
  std::optional<ai_light::ai_light_iteration> get_ai_light_iteration(
      const uuid& in_session_id, std::int32_t in_iteration
  );
  /// 按分布式任务反查轮次（工作机报完结果后回填用）
  std::optional<ai_light::ai_light_iteration> get_ai_light_iteration_by_task(const uuid& in_server_task_info_id);

  /// 一轮的全部候选，candidate_index 升序
  std::vector<ai_light::ai_light_candidate> get_ai_light_candidates(const uuid& in_iteration_id);
  /// 会话的全部候选（跨轮），按轮次与候选序号升序
  std::vector<ai_light::ai_light_candidate> get_ai_light_candidates_by_session(const uuid& in_session_id);
  /// 被选中作为下一轮基准的候选（warm start 起点）
  std::optional<ai_light::ai_light_candidate> get_ai_light_selected_candidate(
      const uuid& in_session_id, std::int32_t in_iteration
  );
  /// 定稿候选（每会话至多一个）。
  ///
  /// 库层约束由 ai_light_candidate.accepted_session_id 上的**唯一索引**承担：
  /// 该列可空（非定稿行为 NULL，SQLite 唯一索引不约束 NULL），定稿时写入会话 id。
  /// 原方案想用 `WHERE is_accepted` 的 partial unique index，但本库的
  /// column_operations.h 生成 partial index 时把条件硬编码成 `{} == ?` 绑定参数，
  /// SQLite 直接拒绝（parameters prohibited in partial index WHERE clauses），
  /// 而 create_trigger_t 又不支持 WHEN/RAISE。所以改用可空唯一列 —— 效果等价，
  /// 且写入必须经 ai_light_candidate::mark_accepted()，不能直接赋值绕过。
  std::optional<ai_light::ai_light_candidate> get_ai_light_accepted_candidate(const uuid& in_session_id);

  /// 预设库。enabled_only = true 时只取启用的。
  /// 检索本身（余弦相似度）在内存里做：sqlite 没有向量索引，
  /// 而首批规模是几十条，全表扫描完全够用。
  std::vector<ai_light::ai_light_preset> get_ai_light_presets(bool in_enabled_only = true);
  std::optional<ai_light::ai_light_preset> get_ai_light_preset(const uuid& in_preset_id);
};
}  // namespace doodle
