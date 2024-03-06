//
// Created by TD on 2024/2/27.
//

#include "server_task_info.h"

#include <doodle_core/core/core_set.h>
namespace doodle {
std::string server_task_info::read_log(level::level_enum in_level) const {
  auto l_path = get_log_path(in_level);
  if (!FSys::exists(l_path)) return "";
  FSys::ifstream l_file{l_path};
  return std::string{std::istream_iterator<char>{l_file}, std::istream_iterator<char>{}};
}
FSys::path server_task_info::get_log_path(level::level_enum in_level) const {
  return core_set::get_set().get_cache_root(log_path_) / fmt::format("{}.log.txt", magic_enum::enum_name(in_level));
}
void server_task_info::write_log(level::level_enum in_level, std::string_view in_msg) {
  FSys::ofstream{get_log_path(in_level), std::ios::app | std::ios::binary} << in_msg;
}
}  // namespace doodle