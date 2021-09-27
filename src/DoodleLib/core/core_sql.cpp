
#include <DoodleLib/Exception/exception.h>
#include <DoodleLib/Metadata/project.h>
#include <DoodleLib/core/core_set.h>
#include <DoodleLib/core/core_sql.h>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle {
core_sql::core_sql()
    : config(new_object<sqlpp::mysql::connection_config>()) {
  Init();
}

void core_sql::Init() {
  auto& set = core_set::getSet();

  config->port     = set.getSqlPort();
  config->host     = set.getSqlHost();
  config->user     = set.getSqlUser();
  config->password = set.getSqlPassword();
#ifdef NDEBUG
  config->debug    = false;
  config->database = "doodle";
#else
  // config->debug    = true;
  config->database = "doodle";
#endif
}

ConnPtr core_sql::getConnection() const {
  return std::make_unique<sqlpp::mysql::connection>(config);
}
core_sql& core_sql::Get() {
  static core_sql install;
  return install;
}

}
