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

}  // namespace doodle