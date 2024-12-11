//
// Created by TD on 2022/8/4.
//

#include "rules.h"

#include <doodle_core/lib_warp/std_fmt_bitset.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <array>
#include <mutex>
#include <utility>
#include <vector>

namespace doodle::business {

void to_json(nlohmann::json& j, const rules& p) {
  j["work_weekdays"]      = p.work_weekdays_p;
  j["work_pair"]          = p.work_pair_p;
  j["absolute_deduction"] = p.absolute_deduction;
}
void from_json(const nlohmann::json& j, rules& p) {
  j.at("work_weekdays").get_to(p.work_weekdays_p);
  j.at("work_pair").get_to(p.work_pair_p);

  if (j.contains("absolute_deduction"))
    j.at("absolute_deduction").get_to(p.absolute_deduction);
  else
    p.absolute_deduction = rules::get_default().absolute_deduction;
}

rules::rules() = default;
void rules::work_weekdays(const rules::work_day_type& in_work_weekdays) { work_weekdays_p = in_work_weekdays; }
const rules::work_day_type& rules::work_weekdays() const { return work_weekdays_p; }
bool rules::is_work_day(const chrono::weekday& in_time) const { return work_weekdays_p[in_time.c_encoding()]; }
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

rules::work_day_type& rules::work_weekdays() { return work_weekdays_p; }

std::string rules::fmt_str() const {
  return fmt::format(
      "规则 周六日规则 {} 每日规则 {} 额外规则 {} ", work_weekdays_p, fmt::join(work_pair_p, ", "),
      fmt::join(extra_p, ", ")
  );
}
rules::~rules() = default;

}  // namespace doodle::business
