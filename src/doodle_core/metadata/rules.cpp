//
// Created by TD on 2022/8/4.
//

#include "rules.h"

#include <doodle_core/lib_warp/std_fmt_bitset.h>
#include <doodle_core/metadata/detail/time_point_info.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include "metadata/rules.h"
#include <array>
#include <mutex>
#include <utility>
#include <vector>

namespace doodle::business {

void to_json(nlohmann::json& j, const rules& p) {
  j["work_weekdays"]      = p.work_weekdays_p;
  j["work_pair"]          = p.work_pair_p;
  j["extra_holidays"]     = p.extra_holidays_p;
  j["extra_work"]         = p.extra_work_p;
  j["extra_rest"]         = p.extra_rest_p;
  j["absolute_deduction"] = p.absolute_deduction;
}
void from_json(const nlohmann::json& j, rules& p) {
  j.at("work_weekdays").get_to(p.work_weekdays_p);
  j.at("work_pair").get_to(p.work_pair_p);
  j.at("extra_holidays").get_to(p.extra_holidays_p);
  j.at("extra_work").get_to(p.extra_work_p);
  j.at("extra_rest").get_to(p.extra_rest_p);
  if (j.contains("absolute_deduction"))
    j.at("absolute_deduction").get_to(p.absolute_deduction);
  else
    p.absolute_deduction = rules::get_default().absolute_deduction;
}

using namespace rules_ns;
rules::rules() = default;
void rules::work_weekdays(const rules::work_day_type& in_work_weekdays) { work_weekdays_p = in_work_weekdays; }
const rules::work_day_type& rules::work_weekdays() const { return work_weekdays_p; }
void rules::add_work_time(const chrono::seconds& in_begin, const chrono::seconds& in_end) {
  work_pair_p.emplace_back(in_begin, in_end);
}
const std::vector<std::pair<chrono::seconds, chrono::seconds>>& rules::work_time() const { return work_pair_p; }
void rules::add_extra_work(const time_point_wrap& in_begin, const time_point_wrap& in_end, const std::string& in_info) {
  extra_work_p.emplace_back(in_begin, in_end, in_info);
}
const std::vector<rules_ns::time_point_info>& rules::extra_work() const { return extra_work_p; }
void rules::add_extra_rest(const time_point_wrap& in_begin, const time_point_wrap& in_end, const std::string& in_info) {
  extra_rest_p.emplace_back(in_begin, in_end, in_info);
}
const std::vector<rules_ns::time_point_info>& rules::extra_rest() const { return extra_rest_p; }
const rules& rules::get_default() {
  static rules l_rules{};
  static std::once_flag l_f1;
  std::call_once(l_f1, [&]() {
    l_rules.work_weekdays_p = work_Monday_to_Friday;
    l_rules.work_pair_p.emplace_back(9h, 12h);
    l_rules.work_pair_p.emplace_back(13h, 18h);

    // 周六
    l_rules.absolute_deduction[0].emplace_back(12h, 13h);
    l_rules.absolute_deduction[0].emplace_back(18h + 30min, 19h);

    // 周一到周五
    l_rules.absolute_deduction[1].emplace_back(12h, 13h);
    l_rules.absolute_deduction[1].emplace_back(18h, 18h + 30min);

    l_rules.absolute_deduction[2].emplace_back(12h, 13h);
    l_rules.absolute_deduction[2].emplace_back(18h, 18h + 30min);

    l_rules.absolute_deduction[3].emplace_back(12h, 13h);
    l_rules.absolute_deduction[3].emplace_back(18h, 18h + 30min);

    l_rules.absolute_deduction[4].emplace_back(12h, 13h);
    l_rules.absolute_deduction[4].emplace_back(18h, 18h + 30min);

    l_rules.absolute_deduction[5].emplace_back(12h, 13h);
    l_rules.absolute_deduction[5].emplace_back(18h, 18h + 30min);

    // 周日
    l_rules.absolute_deduction[6].emplace_back(12h, 13h);
    l_rules.absolute_deduction[6].emplace_back(18h + 30min, 19h);
  });
  return l_rules;
}

std::string rules::debug_print() {
  return fmt::format(
      "规则 周六日规则 {} 每日规则 {}  节假日规则 {}  调休规则 {}  加班规则 {}", work_weekdays_p,
      fmt::join(work_pair_p, "->"), fmt::join(extra_holidays_p, "->"), fmt::join(extra_rest_p, "->"),
      fmt::join(extra_work_p, "->")
  );
}
void rules::add_extra_holidays(const time_point_wrap& in_begin, const time_point_wrap& in_end) {
  extra_holidays_p.emplace_back(in_begin, in_end);
}
const rules::time_point_vector& rules::extra_holidays() const { return extra_holidays_p; }
rules::work_day_type& rules::work_weekdays() { return work_weekdays_p; }
rules::time_duration_vector& rules::work_time() { return work_pair_p; }
rules::time_point_vector& rules::extra_holidays() { return extra_holidays_p; }
rules::time_point_vector& rules::extra_work() { return extra_work_p; }
rules::time_point_vector& rules::extra_rest() { return extra_rest_p; }
std::string rules::fmt_str() const {
  return fmt::format(
      "规则 周六日规则 {} 每日规则 {}  节假日规则 {}  调休规则 {}  加班规则 {}", work_weekdays_p,
      fmt::join(work_pair_p, "->"), fmt::join(extra_holidays_p, "->"), fmt::join(extra_rest_p, "->"),
      fmt::join(extra_work_p, "->")
  );
}
rules::~rules() = default;

}  // namespace doodle::business
