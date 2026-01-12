//
// Created by TD on 24-9-12.
//

#pragma once

#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/scan_data_t.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/lockfree/spsc_queue.hpp>

#include <optional>
#include <vector>

namespace doodle {
struct preview_file;
struct preview_files_for_entity_t;
struct status_automation;
enum class server_task_info_type;
struct asset_type;
class server_task_info;
struct todo_t;
struct project_and_status_t;
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
}  // namespace doodle
namespace doodle {

struct sqlite_database_impl;
class sqlite_database {
  logger_ptr logger_;

 public:
  std::shared_ptr<sqlite_database_impl> impl_;

  std::vector<uuid> get_temporal_type_ids();

 public:
  sqlite_database()  = default;
  ~sqlite_database() = default;
  /**
   * 这回调函数用于加载数据库,  并且将数据库中的id分配到 sql_id 池中,  以便后续操作,
   * @warning 只有这里会分配id,  之后的操作不会分配, 只会查找id是否为 0 作为插入和更新的依据,
   * 并且在插入id的时候会自动更新为实际id
   * @param in_path 输入的数据库路径
   */
  void load(const FSys::path& in_path);

  /// 备份数据库
  boost::asio::awaitable<void> backup(FSys::path in_path);

  template <typename T>
  std::vector<T> get_all();

  template <typename T>
  std::int64_t uuid_to_id(const uuid& in_uuid);

  template <typename T>
  uuid id_to_uuid(std::int64_t in_id);

  template <typename T>
  T get_by_uuid(const uuid& in_uuid);

  template <typename T>
  boost::asio::awaitable<void> install(const std::shared_ptr<T>& in_data);
  template <typename T>
  boost::asio::awaitable<void> update(const std::shared_ptr<T>& in_data);
  /**
   *
   * @tparam T 任意优化类别
   * @param in_data 传入的数据
   * @return 插入的id(不包含更新的id)
   */
  template <typename T>
  boost::asio::awaitable<void> install_range(const std::shared_ptr<std::vector<T>>& in_data);
  template <typename T>
  boost::asio::awaitable<void> install_range(std::vector<T>* in_data);
  template <typename T>
  boost::asio::awaitable<void> update_range(const std::shared_ptr<std::vector<T>>& in_data);
  template <typename T>
  boost::asio::awaitable<void> update_range(std::vector<T>* in_data);

  template <typename T>
  boost::asio::awaitable<void> remove(const std::vector<std::int64_t>& in_data);
  template <typename T>
  boost::asio::awaitable<void> remove(const std::vector<std::int32_t>& in_data);
  template <typename T>
  boost::asio::awaitable<void> remove(const std::int64_t& in_data);
  template <typename T>
  boost::asio::awaitable<void> remove(const std::vector<uuid>& in_data);
  template <typename T>
  boost::asio::awaitable<void> remove(const uuid& in_data);

  template <typename T>
  std::vector<T> get_by_parent_id(const uuid& in_id);
  boost::asio::awaitable<void> mark_all_notifications_as_read(uuid in_user_id);

  std::vector<attendance_helper::database_t> get_attendance(
      const uuid& in_person_id, const chrono::local_days& in_data
  );
  std::vector<attendance_helper::database_t> get_attendance(
      const uuid& in_person_id, const std::vector<chrono::local_days>& in_data
  );
  std::vector<work_xlsx_task_info_helper::database_t> get_work_xlsx_task_info(
      const uuid& in_person_id, const chrono::local_days& in_data
  );
  std::vector<working_file> get_working_file_by_task(const uuid& in_task_id);
  // working_file get_sim_working_file_by_rig;

  std::int32_t get_notification_count(const uuid& in_user_id);
  std::vector<project_with_extra_data> get_project_for_user(const person& in_user);

  person get_person_for_email(const std::string& in_email);
  /// 获取用户所在的团队对应的项目
  std::vector<project> get_person_projects(const person& in_user);

  /// 获取项目和对应的项目状态
  std::vector<project_and_status_t> get_project_and_status(const person& in_user);

  std::vector<get_comments_t> get_comments(const uuid& in_task_id);
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
  );

  std::vector<entities_and_tasks_t> get_entities_and_tasks(
      const person& in_person, const uuid& in_project_id, const uuid& in_entity_type_id
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
  std::vector<asset_type> get_asset_types_not_temporal_type();
  std::optional<entity_link> get_entity_link(const uuid& in_entity_in_id, const uuid& in_asset_id);
  /// 获取任务额外数据
  std::optional<entity_asset_extend> get_entity_asset_extend(const uuid& in_entity_id);
  /// 获取镜头扩展数据
  std::optional<entity_shot_extend> get_entity_shot_extend(const uuid& in_entity_id);
  /// 获取播放序列对应的实体
  std::vector<playlist_shot> get_playlist_shot_entity(const uuid& in_playlist_id);

  boost::asio::awaitable<void> remove_playlist_shot_for_playlist(const uuid& in_playlist_id);
  std::optional<task_type_asset_type_link> get_task_type_asset_type_link(
      const uuid& in_task_type_id, const uuid& in_asset_type_id
  );
  boost::asio::awaitable<void> remove_task_type_asset_type_link_by_asset_type(const uuid& in_asset_type_id);
  uuid get_project_status_open() const;
  uuid get_project_status_closed() const;

  // 删除孤立的, 没有和其他连接的工作文件
  boost::asio::awaitable<void> delete_working_file_orphaned();
};
}  // namespace doodle