//
// Created by TD on 2022/8/4.
//

#include "rules.h"

#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/detail/time_point_info.h>

namespace doodle {
namespace business {

class rules::impl {
 public:
  using time_point_info = ::doodle::business::rules_ns::time_point_info;

  work_day_type work_weekdays{};
  std::vector<std::pair<
      chrono::seconds,
      chrono::seconds>>
      work_pair{};
  std::vector<std::pair<chrono::seconds, chrono::seconds>> extra_holidays{};
  std::vector<time_point_info> extra_work{};
  std::vector<time_point_info> extra_rest{};

  /// \brief 周六 ->周日(index 6->0)
  constexpr static work_day_type work_Monday_to_Friday{0b0111110};
  constexpr static std::pair<chrono::seconds,
                             chrono::seconds>
      work_9_12{9h, 12h};
  constexpr static std::pair<chrono::seconds,
                             chrono::seconds>
      work_13_18{13h, 18h};
};

void to_json(nlohmann::json& j, const rules& p) {
  j["work_weekdays"]  = p.p_i->work_weekdays;
  j["work_pair"]      = p.p_i->work_pair;
  j["extra_holidays"] = p.p_i->extra_holidays;
  j["extra_work"]     = p.p_i->extra_work;
  j["extra_rest"]     = p.p_i->extra_rest;
}
void from_json(const nlohmann::json& j, rules& p) {
  j.at("work_weekdays").get_to(p.p_i->work_weekdays);
  j.at("work_pair").get_to(p.p_i->work_pair);
  j.at("extra_holidays").get_to(p.p_i->extra_holidays);
  j.at("extra_work").get_to(p.p_i->extra_work);
  j.at("extra_rest").get_to(p.p_i->extra_rest);
}

using namespace rules_ns;
rules::rules()
    : p_i(std::make_unique<impl>()) {
  p_i->work_weekdays = impl::work_Monday_to_Friday;
  p_i->extra_holidays.emplace_back(9h, 12h);
  p_i->extra_holidays.emplace_back(13h, 18h);
}

rules::~rules() = default;

}  // namespace business
}  // namespace doodle
