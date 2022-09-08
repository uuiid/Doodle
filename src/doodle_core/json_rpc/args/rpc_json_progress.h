//
// Created by TD on 2022/5/16.
//
#pragma once

#include <doodle_core/configure/DOODLE_CORE_API.h>
#include <nlohmann/json_fwd.hpp>

#include <cmath>

namespace doodle::json_rpc::args {

class DOODLE_CORE_API rpc_json_progress {
 private:
  friend void to_json(nlohmann::json& nlohmann_json_j, const rpc_json_progress& nlohmann_json_t);
  friend void from_json(const nlohmann::json& nlohmann_json_j, rpc_json_progress& nlohmann_json_t);

 public:
  std::string msg_;
  std::string result_;
  std::double_t progress_;
};
}  // namespace doodle::json_rpc::args
