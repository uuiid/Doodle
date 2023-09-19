//
// Created by td_main on 2023/9/19.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <nlohmann/json.hpp>
#include <string>
namespace doodle {

struct DOODLE_CORE_API msg_error {
  std::int64_t code{};
  std::string message{};
  std::string data{};

  msg_error() = default;
  // to json
  friend void DOODLE_CORE_API to_json(nlohmann::json& nlohmann_json_j, const msg_error& nlohmann_json_t) {
    nlohmann_json_j["code"]    = nlohmann_json_t.code;
    nlohmann_json_j["message"] = nlohmann_json_t.message;
    nlohmann_json_j["data"]    = nlohmann_json_t.data;
  };
  // from json
  friend void DOODLE_CORE_API from_json(const nlohmann::json& nlohmann_json_j, msg_error& nlohmann_json_t) {
    nlohmann_json_j["code"].get_to(nlohmann_json_t.code);
    nlohmann_json_j["message"].get_to(nlohmann_json_t.message);
    nlohmann_json_j["data"].get_to(nlohmann_json_t.data);
  };
};

}  // namespace doodle