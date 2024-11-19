//
// Created by TD on 2024/2/27.
//

#include "server_task_info.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sql.h>
#include <doodle_core/database_task/details/column.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/sqlite3/sqlite3.h>

#include <boost/preprocessor.hpp>

#include <entt/entt.hpp>
#include <sqlpp11/ppgen.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle {

std::string server_task_info::read_log() const {
  auto l_path = get_log_path();
  if (!FSys::exists(l_path)) return "";
  FSys::ifstream l_file{l_path};
  return std::string{std::istream_iterator<char>{l_file}, std::istream_iterator<char>{}};
}
FSys::path server_task_info::get_log_path() const {
  return core_set::get_set().get_cache_root(log_path_).replace_extension(".log.txt");
}
void server_task_info::write_log(std::string_view in_msg) {
  FSys::ofstream{get_log_path(), std::ios::app | std::ios::binary} << in_msg;
}
bool server_task_info::operator==(const doodle::server_task_info& in_rhs) const { return uuid_id_ == in_rhs.uuid_id_; }
}  // namespace doodle