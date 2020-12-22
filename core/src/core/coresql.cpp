/*
 * @Author: your name
 * @Date: 2020-09-10 14:59:48
 * @LastEditTime: 2020-12-14 13:31:03
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\core\coresql.cpp
 */
#include "coresql.h"

#include <Logger.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include <boost/format.hpp>

#include <thread>
#include <stdexcept>
CORE_NAMESPACE_S
coreSql::coreSql()
    : isInit(false),
      ip(),
      dataName(),
      config(std::make_shared<sqlpp::mysql::connection_config>()) {}

void coreSql::initDB(const dstring &ip_str, const dstring &dataName) {
  initDB(ip_str);
  config->database = "test_db";
}

coreSql::~coreSql() = default;

coreSql &coreSql::getCoreSql() {
  static coreSql install;
  return install;
}

void coreSql::initDB() {
  config->user     = "Effects";
  config->password = "Effects";
  config->host     = ip;
  config->port     = 3306;

  //在这里我们使用宏去控制测试数据库
#ifdef NDEBUG
  config->database = "doodle_main";
#else
  config->database = "test_db";
#endif  // defined
  config->debug = false;
}
dstring coreSql::getThreadId() {
  //使用线程id创建不一样的名字
  auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
  boost::format str("db_%s");
  str % thread_id;

  return str.str();
}

mysqlConnPtr coreSql::getConnection() {
  return std::make_unique<sqlpp::mysql::connection>(config);
}
void coreSql::initDB(const dstring &ip_) {
  ip = ip_;
  initDB();
}

CORE_NAMESPACE_E
