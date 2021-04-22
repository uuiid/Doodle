#pragma once

#include <corelib/core_global.h>

// #include <sqlpp11/sqlpp11.h>
DOODLE_NAMESPACE_S

// SQLPP_ALIAS_PROVIDER(basefile_time)

/*
 * 数据库连接类
 * 全局静态类
 */
class CORE_API coreSql {
 public:
  static coreSql &getCoreSql();
  coreSql &operator=(const coreSql &s) = delete;
  coreSql(const coreSql &s)            = delete;

  ~coreSql();

  void initDB(const std::string &path);
  ConnPtr getConnection();

 private:
  void initDB();
  coreSql();

 private:
  std::shared_ptr<sqlpp::sqlite3::connection_config> config;

  std::string p_path;
};

DOODLE_NAMESPACE_E
