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
  std::vector<work_xlsx_task_info> task_info_;
  boost::uuids::uuid id_;
  chrono::year_month year_month_;
  entt::entity user_refs_;
  chrono::microseconds duration_;
};

namespace work_xlsx_task_info_helper {
struct database_t{
  using zoned_time = chrono::zoned_time<chrono::microseconds>;
  std::int32_t id_{};
  uuid uuid_id_{};
  zoned_time start_time_;
  zoned_time end_time_;
  chrono::microseconds duration_;
  std::optional<std::string> remark_;       // 程序标注
  std::optional<std::string> user_remark_;  // 用户备注

  chrono::days year_month_;
  std::int32_t user_id_;

  boost::uuids::uuid kitsu_task_ref_id_;
};
}  // namespace work_xlsx_task_info_helper

}  // namespace doodle