#pragma once

#include <core_global.h>

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

  void initDB(const dstring &ip_str, const dstring &dataName);
  void initDB(const dstring &ip_);
  mysqlConnPtr getConnection();

 private:
  void initDB();
  dstring getThreadId();
  coreSql();

 private:
  std::shared_ptr<sqlpp::mysql::connection_config> config;

  bool isInit = false;
  dstring ip;
  dstring dataName;
};

DOODLE_NAMESPACE_E
