//
// Created by TD on 2024/2/27.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/json_warp.h>

#include <magic_enum/magic_enum_all.hpp>
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
  // 任务正在执行
  running,
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
        {server_task_info_status::running, "running"},
        {server_task_info_status::completed, "completed"},
        {server_task_info_status::canceled, "canceled"},
        {server_task_info_status::failed, "failed"},
        {server_task_info_status::unknown, "unknown"},
    }
);

enum server_task_info_type {
  // 导出fbx任务
  export_fbx = 0,
  // 导出解算任务
  export_sim,
  // 自动灯光任务
  auto_light,
  // 合成视频
  merge_video,
  // 连接视频
  connect_video,
  // 检查maya文件
  check_maya,
  // 替换maya引用文件
  replace_maya_ref,
};
NLOHMANN_JSON_SERIALIZE_ENUM(
    server_task_info_type,
    {
        {server_task_info_type::export_fbx, "export_fbx"},
        {server_task_info_type::export_sim, "export_sim"},
        {server_task_info_type::auto_light, "auto_light"},
        {server_task_info_type::merge_video, "merge_video"},
        {server_task_info_type::connect_video, "connect_video"},
        {server_task_info_type::check_maya, "check_maya"},
        {server_task_info_type::replace_maya_ref, "replace_maya_ref"},

    }
);
class server_task_info : boost::equality_comparable<server_task_info> {
 public:
  using zoned_time = chrono::zoned_time<chrono::system_clock::duration>;
  std::int32_t id_{};
  // 唯一id
  uuid uuid_id_{};
  // 执行程序
  std::string exe_{};
  // 任务命令
  nlohmann::json command_{};
  // 任务的状态
  server_task_info_status status_{server_task_info_status::submitted};
  // 任务名称
  std::string name_{};

  // 提交任务的计算机
  std::string source_computer_{};
  // 提交人
  uuid submitter_{};
  // 提交时间
  zoned_time submit_time_{};

  uuid run_computer_id_{};

  // 开始运行任务的时间
  std::optional<zoned_time> run_time_{};
  // 结束运行任务的时间
  std::optional<zoned_time> end_time_{};
  nlohmann::json run_time_info_{};

  uuid kitsu_task_id_{};

  server_task_info_type type_{};
  std::string last_line_log_;

  struct run_time_info_t {
    zoned_time start_time_{chrono::current_zone(), chrono::system_clock::now()};
    zoned_time end_time_{};
    std::string info_{};
    // from json
    friend void from_json(const nlohmann::json& j, run_time_info_t& p) {
      j.at("start_time").get_to(p.start_time_);
      j.at("end_time").get_to(p.end_time_);
      j.at("info").get_to(p.info_);
    }
    // to json
    friend void to_json(nlohmann::json& j, const run_time_info_t& p) {
      j["start_time"]    = p.start_time_;
      j["end_time"]      = p.end_time_;
      j["info"] = p.info_;
    }
  };
  std::vector<run_time_info_t> get_run_time_info() const {
    if (run_time_info_.is_array()) return run_time_info_.get<std::vector<run_time_info_t>>();
    return {};
  }
  void set_run_time_info(const std::vector<run_time_info_t>& in_run_time_info) { run_time_info_ = in_run_time_info; }
  void add_run_time_info(const run_time_info_t& in_run_time_info) { run_time_info_.push_back(in_run_time_info); }

  static constexpr auto logger_category = "server_task";

  void get_last_line_log();
  void clear_log_file();

  bool operator==(const server_task_info& in_rhs) const {
    return std::tie(
               id_, uuid_id_, exe_, command_, status_, name_, source_computer_, submitter_, submit_time_,
               run_computer_id_, run_time_, end_time_, kitsu_task_id_, type_
           ) ==
           std::tie(
               in_rhs.id_, in_rhs.uuid_id_, in_rhs.exe_, in_rhs.command_, in_rhs.status_, in_rhs.name_,
               in_rhs.source_computer_, in_rhs.submitter_, in_rhs.submit_time_, in_rhs.run_computer_id_,
               in_rhs.run_time_, in_rhs.end_time_, in_rhs.kitsu_task_id_, in_rhs.type_
           );
  }

  void sql_command(const std::string& in_str);
  const std::string& sql_command() const;

 private:
  mutable std::string sql_command_cache_;
  // to json
  friend void to_json(nlohmann::json& j, const server_task_info& p) {
    j["id"]              = p.uuid_id_;
    j["status"]          = p.status_;
    j["name"]            = p.name_;
    j["source_computer"] = p.source_computer_;
    j["submitter"]       = p.submitter_;
    j["submit_time"]     = p.submit_time_;
    j["run_time"]        = p.run_time_;
    j["end_time"]        = p.end_time_;
    j["run_computer_id"] = p.run_computer_id_;
    j["type"]            = p.type_;
    j["last_line_log"]   = p.last_line_log_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, server_task_info& p) {
    j.at("name").get_to(p.name_);
    j.at("source_computer").get_to(p.source_computer_);
    if (j.contains("submitter")) j.at("submitter").get_to(p.submitter_);
    if (j.contains("run_computer_id")) j.at("run_computer_id").get_to(p.run_computer_id_);
    j.at("type").get_to(p.type_);
  }
};
}  // namespace doodle