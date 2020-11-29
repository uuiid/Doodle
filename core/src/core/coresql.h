#pragma once

#include <core_global.h>

CORE_NAMESPACE_S

/*
 * 数据库连接类
 * 全局静态类
 */
class CORE_API coreSql {
 public:
  static coreSql &getCoreSql();
  coreSql &operator=(const coreSql &s) = delete;
  coreSql(const coreSql &s) = delete;

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

CORE_NAMESPACE_E
