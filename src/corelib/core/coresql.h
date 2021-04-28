#pragma once

#include <corelib/core_global.h>

// #include <sqlpp11/sqlpp11.h>
DOODLE_NAMESPACE_S

// SQLPP_ALIAS_PROVIDER(basefile_time)

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
 public:
  coreSql();
  DOODLE_DISABLE_COPY(coreSql)

  void initDB(sqlOpenMode flags);
  ConnPtr getConnection();
  ConnPtr getConnection(sqlOpenMode flags);

 private:
  void initDB();

 private:
  std::shared_ptr<sqlpp::sqlite3::connection_config> config;

  std::string p_path;
};

DOODLE_NAMESPACE_E