//
// Created by TD on 24-9-12.
//

#pragma once

#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/scan_data_t.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/lockfree/spsc_queue.hpp>

#include <tl/expected.hpp>

namespace doodle {
enum server_task_info_type : int;
struct asset_type;
class server_task_info;
struct todo_t;
struct project_and_status_t;
struct get_comments_t;
struct task_status;
struct assets_and_tasks_t;
struct entities_and_tasks_t;
struct department;
struct task;
}  // namespace doodle
namespace doodle {
namespace attendance_helper {
struct database_t;
}
namespace assets_file_helper {
struct database_t;
}
namespace work_xlsx_task_info_helper {
struct database_t;
}

struct sqlite_database_impl;
class sqlite_database {
  std::shared_ptr<sqlite_database_impl> impl_;

  std::vector<uuid> get_temporal_type_ids();
  void todo_post_processing(std::vector<todo_t>& in);

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
  /**
   *
   * @tparam T 任意优化类别
   * @param in_data 传入的数据
   * @return 插入的id(不包含更新的id)
   */
  template <typename T>
  boost::asio::awaitable<void> install_range(const std::shared_ptr<std::vector<T>>& in_data);

  template <typename T>
  boost::asio::awaitable<void> remove(const std::shared_ptr<std::vector<std::int64_t>>& in_data);
  template <typename T>
  boost::asio::awaitable<void> remove(const std::shared_ptr<uuid>& in_data);

  template <typename T>
  std::vector<T> get_by_parent_id(const uuid& in_id);

  std::vector<attendance_helper::database_t> get_attendance(
      const std::int64_t& in_ref_id, const chrono::local_days& in_data
  );
  std::vector<attendance_helper::database_t> get_attendance(
      const std::int64_t& in_ref_id, const std::vector<chrono::local_days>& in_data
  );
  std::vector<work_xlsx_task_info_helper::database_t> get_work_xlsx_task_info(
      const std::int64_t& in_ref_id, const chrono::local_days& in_data
  );

  std::vector<server_task_info> get_server_task_info(const uuid& in_computer_id);
  std::vector<server_task_info> get_server_task_info_by_user(const uuid& in_user_id);
  std::vector<server_task_info> get_server_task_info_by_type(const server_task_info_type& in_user_id);

  std::vector<project_helper::database_t> find_project_by_name(const std::string& in_name);
  std::int32_t get_notification_count(const uuid& in_user_id);
  std::vector<project_with_extra_data> get_project_for_user(const person& in_user);

  person get_person_for_email(const std::string& in_email);
  /// 获取用户所在的团队对应的项目
  std::vector<project> get_person_projects(const person& in_user);
  /// 获取用户需要做的任务
  std::vector<todo_t> get_person_tasks(const person& in_user, bool is_done = false);
  std::vector<todo_t> get_preson_tasks_to_check(const person& in_user);
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
  // 查询对应的task是否存在
  bool is_task_exist(const uuid& in_entity_id, const uuid& in_task_type_id);

  task_status get_task_status_by_name(const std::string& in_name);
  asset_type get_entity_type_by_name(const std::string& in_name);

  // 获取用户的订阅
  std::set<uuid> get_person_subscriptions(
      const person& in_person, const uuid& in_project_id, const uuid& in_asset_type_uuid
  );
  /**
   *
   * @param in_person 传入的角色
   * @param in_project_id 传入的项目id
   * @param in_id 传入的资产准确id从逻辑上和项目id互斥
   * @return 查询结果
   */
  std::vector<assets_and_tasks_t> get_assets_and_tasks(
      const person& in_person, const uuid& in_project_id, const uuid& in_id = {}
  );
  std::vector<entities_and_tasks_t> get_entities_and_tasks(
      const person& in_person, const uuid& in_project_id, const uuid& in_entity_type_id
  );
  std::set<uuid> get_notification_recipients(const task& in_task);
};
}  // namespace doodle