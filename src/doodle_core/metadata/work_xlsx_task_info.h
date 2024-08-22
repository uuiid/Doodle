#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
class work_xlsx_task_info {
 public:
  // work_xlsx_task_info()  = default;
  // ~work_xlsx_task_info() = default;
  using zoned_time = chrono::zoned_time<chrono::microseconds>;
  boost::uuids::uuid id_;
  zoned_time start_time_;
  zoned_time end_time_;
  chrono::microseconds duration_;
  std::string remark_;
  std::string user_remark_;

  boost::uuids::uuid kitsu_task_ref_id_;

  // to json
  friend void to_json(nlohmann::json& j, const work_xlsx_task_info& p);
};

class work_xlsx_task_info_block {
 public:
  // work_xlsx_task_info_block()  = default;
  // ~work_xlsx_task_info_block() = default;

  std::vector<work_xlsx_task_info> task_info_;
  boost::uuids::uuid id_;
  chrono::year_month year_month_;
  entt::entity user_refs_;
  chrono::microseconds duration_;

  // 传入user的uuid 和 实体对应数据
  static std::vector<work_xlsx_task_info_block> select_all(
      const sql_connection_ptr& in_comm, const std::map<boost::uuids::uuid, entt::entity>& in_map_id
  );
  static void create_table(const sql_connection_ptr& in_comm);

  // 过滤已经存在的任务
  static std::vector<bool> filter_exist(
      const sql_connection_ptr& in_comm, const std::vector<work_xlsx_task_info_block>& in_task
  );
  static void insert(
      const sql_connection_ptr& in_comm, const std::vector<work_xlsx_task_info_block>& in_task,
      const std::map<entt::entity, boost::uuids::uuid>& in_map_id
  );
  static void update(const sql_connection_ptr& in_comm, const std::vector<work_xlsx_task_info_block>& in_task);
  static void delete_by_ids(const sql_connection_ptr& in_comm, const std::vector<boost::uuids::uuid>& in_ids);
};

}  // namespace doodle