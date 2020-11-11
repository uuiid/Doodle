#pragma once

#include "core_global.h"

CORE_NAMESPACE_S

/*
* 数据库连接类
* 全局静态类
*/
class CORE_API coreSql{
 public:
  static coreSql &getCoreSql();
  coreSql &operator=(const coreSql &s) = delete;
  coreSql(const coreSql &s) = delete;

  ~coreSql();

  void closeDataBase();
  static bool commitDataBase();

  void initDB(const dstring &ip_str, const dstring &dataName);
  void initDB(const dstring &ip_);
  mysqlConnPtr getConnection();
 private:
  void initDB();
  QString getThreadId();
  coreSql();

 private:
  std::shared_ptr<sqlpp::mysql::connection_config> config;

  bool isInit = false;
  std::string ip;
  std::string  dataName;
};

CORE_NAMESPACE_E

