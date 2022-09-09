//
// Created by TD on 2022/9/9.
//

#include "access_token.h"
#include <nlohmann/json.hpp>


namespace doodle {
namespace dingding {

void from_json(const nlohmann::json& j, access_token& p) {
  p.token      = j["access_token"].get<std::string>();
  p.expires_in = j["expires_in"].get<std::int32_t>();
};
}  // namespace dingding
}  // namespace doodle
