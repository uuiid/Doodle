
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sql.h>
#include <doodle_core/exception/exception.h>

#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle {

namespace details {
conn_ptr database_info::get_connection() const {
  sqlpp::sqlite3::connection_config l_config{};
#ifdef NDEBUG
  l_config.debug = false;
#else
  l_config.debug = true;
#endif

  if (!exists(path_))
    l_config.flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  else
    l_config.flags = SQLITE_OPEN_READWRITE;
  l_config.path_to_database = path_.generic_string();
  return std::make_unique<sqlpp::sqlite3::connection>(l_config);
}

conn_ptr database_info::get_connection_const() const {
  sqlpp::sqlite3::connection_config l_config{};
#ifdef NDEBUG
  l_config.debug = false;
#else
  l_config.debug = true;
#endif
  l_config.flags            = SQLITE_OPEN_READONLY;
  l_config.path_to_database = path_.generic_string();
  return std::make_unique<sqlpp::sqlite3::connection>(l_config);
}
}  // namespace details
}  // namespace doodle
