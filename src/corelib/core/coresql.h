#pragma once

#include <corelib/core_global.h>

// #include <sqlpp11/sqlpp11.h>
DOODLE_NAMESPACE_S

/*
 * 数据库连接类
 * 全局静态类
 */
enum class sqlOpenMode{
  readOnly,
  write,
  create
};


class CORE_API coreSql {
  Project* p_project;
  std::shared_ptr<sqlpp::sqlite3::connection_config> config;

 public:
  explicit coreSql(Project* in_project);
  DOODLE_DISABLE_COPY(coreSql)

  void initDB(sqlOpenMode flags);
//  [[nodiscard]] ConnPtr getConnection() const;
//  [[nodiscard]] ConnPtr getConnection(sqlOpenMode flags);

};

DOODLE_NAMESPACE_E