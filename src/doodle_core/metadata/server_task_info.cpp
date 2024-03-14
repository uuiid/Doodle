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
bool server_task_info::operator==(const doodle::server_task_info &in_rhs) const {
  return std::tie(
             data_, status_, name_, source_computer_, submitter_, submit_time_, run_computer_, run_computer_ip_,
             run_time_, end_time_, log_path_
         ) ==
         std::tie(
             in_rhs.data_, in_rhs.status_, in_rhs.name_, in_rhs.source_computer_, in_rhs.submitter_,
             in_rhs.submit_time_, in_rhs.run_computer_, in_rhs.run_computer_ip_, in_rhs.run_time_, in_rhs.end_time_,
             in_rhs.log_path_
         );
}
}  // namespace doodle