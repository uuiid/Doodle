//
// Created by TD on 2024/2/27.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <magic_enum.hpp>
#include <nlohmann/json.hpp>
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
  // 任务已经被接受
  accepted,
  // 任务已经被拒绝
  rejected,
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
        {server_task_info_status::accepted, "accepted"},
        {server_task_info_status::rejected, "rejected"},
        {server_task_info_status::completed, "completed"},
        {server_task_info_status::canceled, "canceled"},
        {server_task_info_status::failed, "failed"},
        {server_task_info_status::unknown, "unknown"},
    }
);
class server_task_info : boost::equality_comparable<server_task_info> {
 public:
  server_task_info() = default;
  explicit server_task_info(nlohmann::json in_data) : data_(std::move(in_data)) {}
  ~server_task_info() = default;

  // 唯一id
  boost::uuids::uuid id_{};

  nlohmann::json data_{};
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

  // 运行任务的计算机名称
  std::string run_computer_{};
  // 运行任务的计算机ip
  std::string run_computer_ip_{};

  // 开始运行任务的时间
  chrono::sys_time_pos run_time_{};
  // 结束运行任务的时间
  chrono::sys_time_pos end_time_{};

  // 任务日志储存
  FSys::path log_path_{};

  std::string read_log(level::level_enum in_level) const;
  FSys::path get_log_path(level::level_enum in_level) const;
  void write_log(level::level_enum in_level, std::string_view in_msg);
  bool operator==(const server_task_info& in_rhs) const;

  void install_db(const conn_ptr& in_comm) const;
  void delete_db(const conn_ptr& in_comm) const;
  void update_db(const conn_ptr& in_comm) const;
  void select_db(const conn_ptr& in_comm);

  static std::vector<server_task_info> select_all(const conn_ptr& in_comm);

 private:
  // to json
  friend void to_json(nlohmann::json& j, const server_task_info& p) {
    j["data"]            = p.data_;
    j["status"]          = p.status_;
    j["name"]            = p.name_;
    j["source_computer"] = p.source_computer_;
    j["submitter"]       = p.submitter_;
    j["submit_time"]     = fmt::to_string(p.submit_time_);
    j["run_computer"]    = p.run_computer_;
    j["run_computer_ip"] = p.run_computer_ip_;
    j["run_time"]        = fmt::to_string(p.run_time_);
    j["end_time"]        = fmt::to_string(p.end_time_);
  }
};
}  // namespace doodle