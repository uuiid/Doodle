//
// Created by TD on 2024/2/27.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <strstream>
namespace doodle {

/**
 * 任务的状态,
 * 提交 -> 分配 -> 接受 -> 完成/失败
 * 提交 -> 分配 -> 拒绝
 */

enum class server_task_info_status {
  // 任务已经提交
  submitted,
  // 任务已经分配
  assigned,
  // 任务已经被完成
  completed,
  // 任务已经被取消
  canceled,
  // 任务已经失败
  failed,
  unknown,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    server_task_info_status,
    {
        {server_task_info_status::submitted, "submitted"},
        {server_task_info_status::assigned, "assigned"},
        {server_task_info_status::completed, "completed"},
        {server_task_info_status::canceled, "canceled"},
        {server_task_info_status::failed, "failed"},
        {server_task_info_status::unknown, "unknown"},
    }
);
class server_task_info : boost::equality_comparable<server_task_info> {
 public:
  std::int32_t id_{};
  // 唯一id
  boost::uuids::uuid uuid_id_{};
  // 执行程序
  std::string exe_{};
  // 任务命令
  std::vector<std::string> command_{};
  // 任务的状态
  server_task_info_status status_{server_task_info_status::submitted};
  // 任务名称
  std::string name_{};

  // 提交任务的计算机
  std::string source_computer_{};
  // 提交人
  std::string submitter_{};
  // 提交时间
  chrono::sys_time_pos submit_time_{};

  uuid run_computer_id_{};

  // 开始运行任务的时间
  chrono::sys_time_pos run_time_{};
  // 结束运行任务的时间
  chrono::sys_time_pos end_time_{};

  static constexpr auto logger_category = "server_task";

  std::string read_log() const;
  FSys::path get_log_path() const;
  void write_log(std::string_view in_msg);
  bool operator==(const server_task_info& in_rhs) const;

  void sql_command(const std::string& in_str);
  const std::string& sql_command() const;

 private:
  mutable std::string sql_command_cache_;
  // to json
  friend void to_json(nlohmann::json& j, const server_task_info& p) {
    j["id"]              = fmt::to_string(p.id_);
    j["exe"]             = p.exe_;
    j["command"]         = p.command_;
    j["status"]          = p.status_;
    j["name"]            = p.name_;
    j["source_computer"] = p.source_computer_;
    j["submitter"]       = p.submitter_;
    j["submit_time"]     = fmt::to_string(p.submit_time_);
    j["run_time"]        = fmt::to_string(p.run_time_);
    j["end_time"]        = fmt::to_string(p.end_time_);
    j["run_computer_id"] = fmt::to_string(p.run_computer_id_);
  }
  // from json
  friend void from_json(const nlohmann::json& j, server_task_info& p) {
    j.at("id").get_to(p.uuid_id_);
    j.at("exe").get_to(p.exe_);
    j.at("command").get_to(p.command_);
    j.at("status").get_to(p.status_);
    j.at("name").get_to(p.name_);
    j.at("source_computer").get_to(p.source_computer_);
    j.at("submitter").get_to(p.submitter_);
    j.at("run_computer_id").get_to(p.run_computer_id_);
  }
};
}  // namespace doodle