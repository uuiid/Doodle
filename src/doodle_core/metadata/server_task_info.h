//
// Created by TD on 2024/2/27.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <nlohmann/json.hpp>
namespace doodle {
class server_task_info {
 public:
  server_task_info() = default;
  explicit server_task_info(nlohmann::json in_data) : data_(std::move(in_data)) {}
  ~server_task_info() = default;

  nlohmann::json data_{};
};
}  // namespace doodle