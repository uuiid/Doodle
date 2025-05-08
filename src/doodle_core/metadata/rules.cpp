//
// Created by TD on 2022/8/4.
//

#include "rules.h"

#include <array>
#include <mutex>
#include <vector>

namespace doodle::business {

rules::rules() = default;

const rules& rules::get_default() {
  static rules l_rules{};
  static std::once_flag l_f1;
  std::call_once(l_f1, [&]() {
    l_rules.work_pair_p.emplace_back(9h, 12h);
    l_rules.work_pair_p.emplace_back(13h, 18h);

    // 周六日
    l_rules.work_pair_0_.emplace_back(12h, 13h);
    l_rules.work_pair_0_.emplace_back(18h + 30min, 19h);

    // 周一到周五
    l_rules.work_pair_1_.emplace_back(12h, 13h);
    l_rules.work_pair_1_.emplace_back(18h, 18h + 30min);
  });
  return l_rules;
}

rules::~rules() = default;

}  // namespace doodle::business
