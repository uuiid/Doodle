#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {

namespace work_xlsx_task_info_helper {
struct database_t {
  using zoned_time = chrono::zoned_time<chrono::microseconds>;
  std::int32_t id_{};
  uuid uuid_id_{};
  zoned_time start_time_;
  zoned_time end_time_;
  chrono::microseconds duration_;
  std::string remark_;       // 程序标注
  std::string user_remark_;  // 用户备注

  chrono::local_days year_month_;
  std::int32_t user_ref_;

  boost::uuids::uuid kitsu_task_ref_id_;
  // to json
  friend void to_json(nlohmann::json& j, const database_t& p) {
    j["id"]                = fmt::to_string(p.uuid_id_);
    j["start_time"]        = fmt::format("{:%FT%T}", p.start_time_.get_local_time());
    j["end_time"]          = fmt::format("{:%FT%T}", p.end_time_.get_local_time());
    j["duration"]          = p.duration_.count();
    j["remark"]            = p.remark_;
    j["user_remark"]       = p.user_remark_;
    j["kitsu_task_ref_id"] = fmt::to_string(p.kitsu_task_ref_id_);
  }
};
}  // namespace work_xlsx_task_info_helper

}  // namespace doodle