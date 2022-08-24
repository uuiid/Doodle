
#include <doodle_core/exception/exception.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sql.h>
#include <sqlpp11/sqlpp11.h>

#include <sqlpp11/sqlite3/sqlite3.h>

namespace doodle {

class core_sql::impl {
 public:
  sqlpp::sqlite3::connection_config config;
};

core_sql::core_sql()
    : p_i(std::make_unique<impl>()) {
#ifdef NDEBUG
  p_i->config.debug = false;
#else
//  p_i->config.debug = true;
#endif
}

conn_ptr core_sql::get_connection(const FSys::path& in_path) {
  if (!exists(in_path))
    p_i->config.flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  else
    p_i->config.flags = SQLITE_OPEN_READWRITE;
  p_i->config.path_to_database = in_path.generic_string();
  return std::make_unique<sqlpp::sqlite3::connection>(p_i->config);
}
core_sql& core_sql::Get() {
  static core_sql install;
  return install;
}
conn_ptr core_sql::get_connection_const(const FSys::path& in_path) const {
  p_i->config.flags            = SQLITE_OPEN_READONLY;
  p_i->config.path_to_database = in_path.generic_string();
  return std::make_unique<sqlpp::sqlite3::connection>(p_i->config);
}
core_sql::~core_sql() = default;

}  // namespace doodle
