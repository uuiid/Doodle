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
  sql_command_cache_ = (nlohmann::json{} = command_).dump();
  return sql_command_cache_;
}
void server_task_info::sql_command(const std::string& in_str) {
  sql_command_cache_ = in_str;
  nlohmann::json::parse(sql_command_cache_).get_to(command_);
}

void server_task_info::get_last_line_log() {
  auto l_path =
      core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", uuid_id_);
  if (!FSys::exists(l_path)) return;
  FSys::ifstream l_ifs(l_path, std::ios::binary | std::ios::ate);
  auto l_size = l_ifs.tellg();
  if (l_size > 510) {
    l_size -= 500;
    l_ifs.seekg(l_size);
  } else
    l_ifs.seekg(0);
  while (std::getline(l_ifs, last_line_log_));
}

}  // namespace doodle