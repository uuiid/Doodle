#include "coresql.h"

#include "Logger.h"


#include <thread>
#include <stdexcept>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

CORE_NAMESPACE_S
coreSql::coreSql()
    : isInit(false),
      ip(),
      dataName(),
      config(std::make_shared<sqlpp::mysql::connection_config>()) {

}

void coreSql::initDB(const QString &ip_, const QString &dataName_) {
  initDB(ip_);
}

coreSql::~coreSql() {
  closeDataBase();
}

coreSql &coreSql::getCoreSql() {
  static coreSql install;
  return install;
}

void coreSql::closeDataBase() {
//  QSqlDatabase::database("mysql_main_data").close();
}

bool coreSql::commitDataBase() {
//  QSqlDatabase dataBase = QSqlDatabase::database("mysql_main_data");
//  dataBase.open();
//  return dataBase.commit();
  return false;
}
void coreSql::initDB() {
  config->user = "Effects";
  config->password = "Effects";
  config->host = ip;
  config->port = 3306;
  config->database = "doodle_main";
  config->debug = true;

  auto db_name = getThreadId();

//  QSqlDatabase dataBase;
//  if (QSqlDatabase::contains(db_name)) {
//    dataBase = QSqlDatabase::database(db_name);
//  } else {
//    dataBase = QSqlDatabase::addDatabase("QMYSQL", db_name);
//    dataBase.setUserName("Effects");
//    dataBase.setPassword("Effects");
//
//    dataBase.setHostName(ip);
//    dataBase.setPort(3306);
//    dataBase.setDatabaseName(dataName);
//  }
//  if (!dataBase.open()) throw std::runtime_error(dataBase.lastError().text().toStdString());
}
QString coreSql::getThreadId() {
  //使用线程id创建不一样的名字
  auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
  auto db_name = QString("%1%2").arg("mysql_main").arg(thread_id);
  return db_name;
}

mysqlConnPtr coreSql::getConnection() {
  return std::make_unique<sqlpp::mysql::connection>(config);
}
void coreSql::initDB(const QString &ip_) {
  ip = ip_.toStdString();
}

CORE_NAMESPACE_E
