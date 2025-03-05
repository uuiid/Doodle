#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {

namespace attendance_helper {
enum class att_enum : std::uint32_t {
  // 加班
  overtime = 0,
  // 请假
  leave    = 1,
  // 最大值
  max      = 2
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    att_enum, {{att_enum::overtime, "overtime"}, {att_enum::leave, "leave"}, {att_enum::max, "max"}}

)

struct database_t {
  std::int32_t id_{};
  uuid uuid_id_{};

  chrono::zoned_time<chrono::microseconds> start_time_;
  chrono::zoned_time<chrono::microseconds> end_time_;
  std::string remark_;
  att_enum type_{att_enum::overtime};
  chrono::local_days create_date_{};                        //
  chrono::zoned_time<chrono::microseconds> update_time_{};  // 更新时间

  std::string dingding_id_{};  // 钉钉id
  std::int64_t user_ref{};
  friend void to_json(nlohmann::json& j, const database_t& p) {
    j["id"]         = p.uuid_id_;
    j["start_time"] = p.start_time_;
    j["end_time"]   = p.end_time_;
    j["remark"]     = p.remark_;
    j["type"]       = p.type_;
    j["is_custom"]  = p.dingding_id_.empty();
  }
  friend void from_json(const nlohmann::json& j, database_t& p) {
    if (j.contains("start_time")) j.at("start_time").get_to(p.start_time_);
    if (j.contains("end_time")) j.at("end_time").get_to(p.end_time_);
    if (j.contains("remark")) j.at("remark").get_to(p.remark_);
    if (j.contains("type")) j.at("type").get_to(p.type_);
    if (j.contains("create_date")) j.at("create_date").get_to(p.create_date_);
  }
};
}  // namespace attendance_helper

}  // namespace doodle