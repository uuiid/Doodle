
#include <DoodleLib/Exception/exception.h>
#include <DoodleLib/Metadata/project.h>
#include <DoodleLib/core/CoreSql.h>
#include <DoodleLib/core/core_set.h>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle {
CoreSql::CoreSql()
    : config(new_object<sqlpp::mysql::connection_config>()) {
  Init();
}

void CoreSql::Init() {
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

ConnPtr CoreSql::getConnection() const {
  return std::make_unique<sqlpp::mysql::connection>(config);
}
CoreSql& CoreSql::Get() {
  static CoreSql install;
  return install;
}

}
