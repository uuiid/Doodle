//
// Created by TD on 2022/10/26.
//

#include "holidaycn_time.h"

#include <doodle_core/time_tool/work_clock.h>

#include <doodle_lib/lib_warp/cmrc.h>

namespace doodle {
void to_json(nlohmann::json &in_j, const holidaycn_time::info &in_p) {
  in_j["name"]       = in_p.name;
  in_j["date"]       = in_p.date;
  in_j["is_odd_day"] = in_p.is_odd_day;
}
void from_json(const nlohmann::json &in_j, holidaycn_time::info &in_p) {
  in_j.at("name").get_to(in_p.name);
  in_j.at("isOffDay").get_to(in_p.is_odd_day);

  auto l_str = std::istringstream{in_j.at("date").get<std::string>()};
  l_str >> chrono::parse("%Y-%m-%d", in_p.date);
}

holidaycn_time::holidaycn_time() {
  auto l_file = cmrc::doodle_holidaycn::get_filesystem().open("2022.json");

  auto l_json = nlohmann::json::parse(std::string_view{l_file.begin(), l_file.size()});
  for (const auto &i : l_json.at("days").get<std::vector<info>>()) {
    if (i.is_odd_day)
      holidaycn_list_rest.emplace_back(
          std::make_tuple(time_point_wrap{i.date}, time_point_wrap{i.date + chrono::days{1}}, i.name)
      );
    else
      holidaycn_list_work.emplace_back(
          std::make_tuple(time_point_wrap{i.date}, time_point_wrap{i.date + chrono::days{1}}, i.name)
      );
  };
}
holidaycn_time::~holidaycn_time() = default;
void holidaycn_time::set_clock(business::work_clock &in_work_clock) const {
  for (const auto &item : holidaycn_list_work) in_work_clock += item;
  for (const auto &item : holidaycn_list_rest) in_work_clock -= item;
}
}  // namespace doodle