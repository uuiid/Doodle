//
// Created by TD on 2024/2/27.
//

#include "server_task_info.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/preprocessor.hpp>

#include <entt/entt.hpp>

namespace doodle {

const std::string& server_task_info::sql_command() const {
  sql_command_cache_ = command_.dump();
  return sql_command_cache_;
}
void server_task_info::sql_command(const std::string& in_str) {
  if (!in_str.empty()) command_ = nlohmann::json::parse(in_str);
}

void server_task_info::get_last_line_log() {
  if (status_ != server_task_info_status::failed) return;
  auto l_path =
      core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", uuid_id_);
  if (!FSys::exists(l_path)) return;
  FSys::ifstream l_ifs(l_path, std::ios::ate);
  auto l_size = l_ifs.tellg();
  if (l_size > 810) {
    l_size -= 800;
    l_ifs.seekg(l_size);
  } else
    l_ifs.seekg(0);
  std::string l_line{};
  while (std::getline(l_ifs, l_line))
    if (!l_line.empty()) last_line_log_ = l_line;
}
void server_task_info::clear_log_file() {
  auto l_path =
      core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", uuid_id_);
  if (!FSys::exists(l_path)) return;
  // 清空日志, 不删除, 防止记事本等打开, 无法删除
  FSys::ofstream{l_path, std::ios::out | std::ios::trunc};
}

}  // namespace doodle