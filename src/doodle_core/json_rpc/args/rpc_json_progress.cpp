//
// Created by TD on 2022/5/16.
//

#include "rpc_json_progress.h"

#include <nlohmann/json.hpp>

namespace doodle::json_rpc::args {
void to_json(nlohmann::json& nlohmann_json_j, const rpc_json_progress& nlohmann_json_t) {
  nlohmann_json_j["result"]   = nlohmann_json_t.result_;
}

void from_json(const nlohmann::json& nlohmann_json_j, rpc_json_progress& nlohmann_json_t) {
  nlohmann_json_j.at("result").get_to(nlohmann_json_t.result_);
}
}  // namespace doodle::json_rpc::args
