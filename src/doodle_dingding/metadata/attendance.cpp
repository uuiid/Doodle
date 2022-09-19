//
// Created by TD on 2022/9/16.
//

#include "attendance.h"
#include <doodle_core/lib_warp/std_warp.h>
#include <fmt/chrono.h>

namespace doodle {
namespace dingding {
namespace attendance {

namespace query {
void to_json(nlohmann::json& nlohmann_json_j, const get_day_data& nlohmann_json_t) {
  /// 时间需要特殊处理

  nlohmann_json_j["work_date_from"] = fmt::format(
      "{}",
      nlohmann_json_t.work_date_from
  );
  nlohmann_json_j["work_date_to"] = fmt::format(
      "{}",
      nlohmann_json_t.work_date_to
  );

  nlohmann_json_j["user_id_list"] = nlohmann_json_t.user_id_list;
  nlohmann_json_j["offset"]       = nlohmann_json_t.offset;
  nlohmann_json_j["limit"]        = nlohmann_json_t.limit;
  nlohmann_json_j["is_i18_n"]     = nlohmann_json_t.is_i18_n;
}
void from_json(const nlohmann::json& nlohmann_json_j, get_day_data& nlohmann_json_t) {
  /// 时间需要特殊处理
  chrono::local_seconds time;
  {
    std::istringstream l_time{nlohmann_json_j.at("work_date_from").get<std::string>()};
    l_time >> chrono::parse("%Y-%m-%d %H:%M:%S", time);
    nlohmann_json_t.work_date_from = time;
  }
  {
    std::istringstream l_time{nlohmann_json_j.at("work_date_to").get<std::string>()};
    l_time >> chrono::parse("%Y-%m-%d %H:%M:%S", time);
    nlohmann_json_t.work_date_to = time;
  }

  nlohmann_json_j.at("user_id_list").get_to(nlohmann_json_t.user_id_list);
  nlohmann_json_j.at("offset").get_to(nlohmann_json_t.offset);
  nlohmann_json_j.at("limit").get_to(nlohmann_json_t.limit);
  nlohmann_json_j.at("is_i18_n").get_to(nlohmann_json_t.is_i18_n);
}
}  // namespace query
}  // namespace attendance

}  // namespace dingding
}  // namespace doodle
