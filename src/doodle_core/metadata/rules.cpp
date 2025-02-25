//
// Created by TD on 2022/8/4.
//

#include "rules.h"

#include <array>
#include <mutex>
#include <vector>

namespace doodle::business {

rules::rules() = default;
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

rules::~rules() = default;

}  // namespace doodle::business
