
#include <corelib/core/coresql.h>
#include <corelib/Metadata/Project.h>
#include <loggerlib/Logger.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <loggerlib/Logger.h>
#include <corelib/Exception/Exception.h>

DOODLE_NAMESPACE_S
coreSql::coreSql(std::weak_ptr<Project> in_project)
    : p_project(std::move(in_project)),
      config(std::make_shared<sqlpp::sqlite3::connection_config>()) {}

ConnPtr coreSql::getConnection() const{
  return std::make_unique<sqlpp::sqlite3::connection>(*config);
}
void coreSql::initDB(sqlOpenMode flags) {
  switch (flags) {
    case sqlOpenMode::readOnly: config->flags = SQLITE_OPEN_READONLY;
      break;
    case sqlOpenMode::write: config->flags = SQLITE_OPEN_READWRITE;
      break;
    case sqlOpenMode::create: config->flags = SQLITE_OPEN_CREATE;
      break;
  }
  if (p_project.expired()) throw DoodleError{"无效prj指针"};
  auto path = p_project.lock()->Path();
  path /= Project::getConfigFileFolder();
  path /= Project::getConfigFileName();
  DOODLE_LOG_INFO("open db: " << path)
  config->path_to_database = path.generic_string();
#ifdef NDEBUG
  config->debug = false;
#else
  config->debug = true;
#endif  //NDEBUG

}
ConnPtr coreSql::getConnection(sqlOpenMode flags) {
  initDB(flags);
  return getConnection();
}

DOODLE_NAMESPACE_E