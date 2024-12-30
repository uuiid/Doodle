//
// Created by TD on 2022/10/26.
//

#include "holidaycn_time.h"

#include "doodle_core/metadata/time_point_wrap.h"
#include <doodle_core/time_tool/work_clock.h>

#include <doodle_lib/lib_warp/cmrc.h>

#include <utility>

namespace doodle {

void to_json(nlohmann::json &in_j, const holidaycn_time2::info &in_p) {
  in_j["name"]       = in_p.name;
  in_j["date"]       = in_p.date;
  in_j["is_odd_day"] = in_p.is_odd_day;
}
void from_json(const nlohmann::json &in_j, holidaycn_time2::info &in_p) {
  in_j.at("name").get_to(in_p.name);
  in_j.at("isOffDay").get_to(in_p.is_odd_day);

  auto l_str = std::istringstream{in_j.at("date").get<std::string>()};
  l_str >> chrono::parse("%Y-%m-%d", in_p.date);
}

void holidaycn_time2::load_year(const FSys::path &in_path) {
  if (!FSys::exists(in_path)) return;

  FSys::ifstream l_file{in_path};
  auto l_json = nlohmann::json::parse(l_file);
  for (const auto &i : l_json.at("days").get<std::vector<info>>()) {
    if (i.is_odd_day)
      holidaycn_list_rest.emplace_back(i.date, i.date + chrono::days{1}, i.name);
    else
      for (auto &&[beg, end] : work_time) {
        holidaycn_list_work.emplace_back(
            chrono::local_time_pos{i.date} + beg, chrono::local_time_pos{i.date} + end, i.name + "调休"
        );
      }
  };
}

holidaycn_time2::holidaycn_time2(time_duration_vector in_work_time, const FSys::path &in_path)
    : work_time(std::move(in_work_time)) {
  auto l_year = chrono::year_month_day{chrono::floor<chrono::days>(
      chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()}.get_local_time()
  )};

  load_year(in_path / fmt::format("{}.json", static_cast<std::int32_t>(l_year.year())));
  load_year(in_path / fmt::format("{}.json", static_cast<std::int32_t>(++l_year.year())));
  load_year(in_path / fmt::format("{}.json", static_cast<std::int32_t>(--l_year.year())));
}

holidaycn_time2::~holidaycn_time2() = default;
void holidaycn_time2::set_clock(business::work_clock2 &in_work_clock) const {
  for (const auto &item : holidaycn_list_work) in_work_clock += item;
  for (const auto &item : holidaycn_list_rest) in_work_clock -= item;
}

}  // namespace doodle