
#include "coresql.h"

#include <loggerlib/Logger.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>

#include <boost/format.hpp>

#include <thread>
#include <stdexcept>
DOODLE_NAMESPACE_S
coreSql::coreSql()
    : p_path(),
      config(std::make_shared<sqlpp::sqlite3::connection_config>()) {}

coreSql::~coreSql() = default;

coreSql &coreSql::getCoreSql() {
  static coreSql install;
  return install;
}

void coreSql::initDB() {
  config->flags            = SQLITE_OPEN_READWRITE;
  config->path_to_database = p_path;
#ifdef NDEBUG
  config->debug = false;
#else
  config->debug = true;
#endif  //NDEBUG
}

mysqlConnPtr coreSql::getConnection() {
  return std::make_unique<sqlpp::sqlite3::connection>(*config);
}
void coreSql::initDB(const dstring &path) {
  p_path = path + "/config/doodleConfig.db";
  initDB();
}

DOODLE_NAMESPACE_E
