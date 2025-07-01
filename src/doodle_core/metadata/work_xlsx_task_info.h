#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {

namespace work_xlsx_task_info_helper {
struct database_t : boost::totally_ordered<database_t> {
  using zoned_time = chrono::zoned_time<chrono::microseconds>;
  std::int32_t id_{};
  uuid uuid_id_{};
  zoned_time start_time_;
  zoned_time end_time_;
  chrono::microseconds duration_;
  std::string remark_;       // 程序标注
  std::string user_remark_;  // 用户备注

  chrono::local_days year_month_;
  uuid person_id_;

  boost::uuids::uuid kitsu_task_ref_id_;

  std::optional<std::int32_t> season_{};   // 季度
  std::optional<std::int32_t> episode_{};  // 集数
  std::string name_{};                     // 任务名称
  std::string grade_{};                    // 任务等级
  uuid project_id_;
  std::string project_name_{};

  friend bool operator<(const database_t& lhs, const database_t& rhs) {
    return lhs.start_time_.get_sys_time() < rhs.start_time_.get_sys_time();
  }
  friend bool operator==(const database_t& lhs, const database_t& rhs) { return lhs.uuid_id_ == rhs.uuid_id_; }

  // to json
  friend void to_json(nlohmann::json& j, const database_t& p) {
    j["id"]          = p.uuid_id_;
    j["start_time"]  = p.start_time_;
    j["end_time"]    = p.end_time_;
    j["duration"]    = p.duration_.count();
    j["remark"]      = p.remark_;
    j["user_remark"] = p.user_remark_;
    j["user_id"]     = p.person_id_;
    if (!p.kitsu_task_ref_id_.is_nil()) j["kitsu_task_ref_id"] = p.kitsu_task_ref_id_;
    if (p.season_) j["season"] = *p.season_;
    if (p.episode_) j["episode"] = *p.episode_;
    if (!p.name_.empty()) j["name"] = p.name_;
    if (!p.grade_.empty()) j["grade"] = p.grade_;
    if (!p.project_id_.is_nil())
      j["project_id"] = p.project_id_;
    else
      j["project_name"] = p.project_name_;
  }
};
}  // namespace work_xlsx_task_info_helper

}  // namespace doodle