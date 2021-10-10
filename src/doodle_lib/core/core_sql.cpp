
#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/core_sql.h>
#include <doodle_lib/metadata/project.h>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle {
core_sql::core_sql()
    : config(new_object<sqlpp::mysql::connection_config>()) {
  Init();
}

void core_sql::Init() {
  auto& set = core_set::getSet();

  config->port     = set.get_sql_port();
  config->host     = set.get_sql_host();
  config->user     = set.get_sql_user();
  config->password = set.get_sql_password();
#ifdef NDEBUG
  config->debug    = false;
  config->database = "doodle";
#else
  // config->debug    = true;
  config->database = "doodle";
#endif
}

conn_ptr core_sql::get_connection() const {
  return std::make_unique<sqlpp::mysql::connection>(config);
}
core_sql& core_sql::Get() {
  static core_sql install;
  return install;
}

}
