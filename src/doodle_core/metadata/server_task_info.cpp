//
// Created by TD on 2024/2/27.
//

#include "server_task_info.h"

#include <doodle_core/doodle_core_fwd.h>

#include <boost/preprocessor.hpp>

#include <entt/entt.hpp>

namespace doodle {

void server_task_info::get_last_line_log(const FSys::path& in_log_path) {
  if (status_ != server_task_info_status::failed) return;
  // auto l_path =
  //     core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", uuid_id_);
  if (!FSys::exists(in_log_path)) return;
  FSys::ifstream l_ifs(in_log_path, std::ios::ate);
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
void server_task_info::clear_log_file(const FSys::path& in_log_path) {
  // auto l_path =
  //     core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", uuid_id_);
  if (!FSys::exists(in_log_path)) return;
  // 清空日志, 不删除, 防止记事本等打开, 无法删除
  FSys::ofstream{in_log_path, std::ios::out | std::ios::trunc};
}

}  // namespace doodle