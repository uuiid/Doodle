//
// Created by TD on 2024/2/27.
//

#include "server_task_info.h"

#include <doodle_core/core/core_set.h>
namespace doodle {
std::string server_task_info::read_log(level::level_enum in_level) const {
  auto l_path =
      core_set::get_set().get_cache_root(log_path_) / fmt::format("{}.log.txt", magic_enum::enum_name(in_level));
  if (!FSys::exists(l_path)) return "";
  FSys::ifstream l_file{l_path};
  return std::string{std::istream_iterator<char>{l_file}, std::istream_iterator<char>{}};
}
void server_task_info::write_log(level::level_enum in_level, std::string_view in_msg) {
  if (!log_file_out_.is_open()) {
    auto l_path =
        core_set::get_set().get_cache_root(log_path_) / fmt::format("{}.log.txt", magic_enum::enum_name(in_level));
    log_file_out_.open(l_path, std::ios::app | std::ios::binary);
  }
  log_file_out_ << in_msg;
}
}  // namespace doodle