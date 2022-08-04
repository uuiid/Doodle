//
// Created by TD on 2022/8/4.
//

#include "rules.h"

namespace doodle {
namespace business {

class rules::impl {
 public:
};

void to_json(nlohmann::json& j, const rules& p) {
  j[""] = p;
}
void from_json(const nlohmann::json& j, rules& p) {
}

rules::rules()
    : p_i(std::make_unique<impl>()) {
}

rules::~rules() = default;

}  // namespace business
}  // namespace doodle
