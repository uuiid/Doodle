
#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/core/CoreSql.h>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>

DOODLE_NAMESPACE_S
CoreSql::CoreSql()
    :      config(std::make_shared<sqlpp::mysql::connection_config>()) {
  sqlpp::mysql::connection_config mysql;
  config->port = 3306;
  config->host = "192.168.10.215";
  config->user = "deve";
  config->password = "deve";
#ifdef NDEBUG
  config->debug = true;
#else
  config->debug = false;
#endif
  config->database = "doodle";
}

ConnPtr CoreSql::getConnection() const{
  return std::make_unique<sqlpp::mysql::connection>(config);
}
CoreSql& CoreSql::Get() {
  static CoreSql install;
  return install;
}

DOODLE_NAMESPACE_E
