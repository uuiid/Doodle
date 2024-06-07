#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/time_point_wrap.h>
namespace doodle {
class attendance {
 public:
  enum class att_enum : std::uint32_t {
    // 加班
    overtime = 0,
    // 请假
    leave    = 1,
  };
  boost::uuids::uuid id_;
  chrono::zoned_time<chrono::microseconds> start_time_;
  chrono::zoned_time<chrono::microseconds> end_time_;
  std::string remark_;
  att_enum type_{att_enum::overtime};

  entt::entity user_ref_id_;

  chrono::zoned_time<chrono::microseconds> update_time_{};  // 更新时间

 public:
  static std::vector<attendance> select_all(
      pooled_connection& in_comm, const std::map<boost::uuids::uuid, entt::entity>& in_map_id
  );
  static void create_table(pooled_connection& in_comm);

  // 过滤已经存在的任务
  static std::vector<bool> filter_exist(pooled_connection& in_comm, const std::vector<attendance>& in_task);
  static void insert(
      pooled_connection& in_comm, const std::vector<attendance>& in_task,
      const std::map<entt::entity, boost::uuids::uuid>& in_map_id
  );
  static void update(pooled_connection& in_comm, const std::vector<attendance>& in_task);
  static void delete_by_ids(pooled_connection& in_comm, const std::vector<boost::uuids::uuid>& in_ids);

  // to_json
  friend void to_json(nlohmann::json& j, const attendance& p);
  // friend void from_json(const nlohmann::json& j, attendance& p);
};
}  // namespace doodle