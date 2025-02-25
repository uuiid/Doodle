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
    j["id"]         = fmt::to_string(p.id_);
    j["start_time"] = p.start_time_;
    j["end_time"]   = p.end_time_;
    j["remark"]     = p.remark_;
    j["type"] = static_cast<std::uint32_t>(p.type_);
  }
};
}  // namespace attendance_helper

}  // namespace doodle