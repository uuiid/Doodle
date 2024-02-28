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

  std::string source_computer_{};
  // 提交人
  std::string submitter_{};
  // 提交时间
  chrono::sys_time_pos submit_time_{};

 private:
  // to json
  friend void to_json(nlohmann::json& j, const server_task_info& p) {
    j["data"]            = p.data_;
    j["source_computer"] = p.source_computer_;
    j["submitter"]       = p.submitter_;
    j["submit_time"]     = p.submit_time_;
  }
};
}  // namespace doodle