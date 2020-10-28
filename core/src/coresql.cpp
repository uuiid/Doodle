#include "coresql.h"

#include "Logger.h"

#include <QSqlError>

#include <QMap>
#include <thread>

CORE_NAMESPACE_S
coreSql::coreSql()
    : isInit(false),
      ip(),
      dataName() {

}

void coreSql::initDB(const QString &ip_, const QString &dataName_) {
  ip = QString(ip);
  dataName = QString(dataName);
}

coreSql::~coreSql() {
  closeDataBase();
}

coreSql &coreSql::getCoreSql() {
  static coreSql install;
  return install;
}

sqlQuertPtr coreSql::getquery() {
  auto db_name = getThreadId();
  initDB();
  QSqlDatabase dataBase = QSqlDatabase::database(db_name);
  dataBase.open();
//    dataBase.transaction();
//  if (!isInit) throw std::runtime_error("not init_ DB (sql mian data)");
  sqlQuertPtr queryPtr(new QSqlQuery(dataBase));
  return queryPtr;
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

  auto db_name = getThreadId();

  QSqlDatabase dataBase;
  if (QSqlDatabase::contains(db_name)) {
    dataBase = QSqlDatabase::database(db_name);
  } else {
    dataBase = QSqlDatabase::addDatabase("QMYSQL", db_name);
    dataBase.setUserName("Effects");
    dataBase.setPassword("Effects");

    dataBase.setHostName(ip);
    dataBase.setPort(3306);
    dataBase.setDatabaseName(dataName);
  }
  if (!dataBase.open()) throw std::runtime_error(dataBase.lastError().text().toStdString());
}
QString coreSql::getThreadId() {
  //使用线程id创建不一样的名字
  auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
  auto db_name = QString("%1%2").arg("mysql_main").arg(thread_id);
  return db_name;
}

coreSqlUser &coreSqlUser::getCoreSqlUser() {
  static coreSqlUser install;
  return install;
}

mapStringPtr coreSqlUser::getUser() {
  QSqlDatabase dataBase = QSqlDatabase::database("mysql_main_user");
  dataBase.open();
  if (!isInit) throw std::runtime_error("not init_ DB (sql mian data)");
  QSqlQuery query(dataBase);

  query.exec("SELECT user,password FROM user;");
  mapStringPtr list;
  while (query.next()) {
    QString user = query.value(0).toString();
    QString powr = query.value(1).toString();
    list.insert(user, powr);
  }
  return list;
}

void coreSqlUser::initDB(const QString &ip, const QString &dataName) {
  QSqlDatabase dataBase;
  if (QSqlDatabase::contains("mysql_main_user")) {
    dataBase = QSqlDatabase::database("mysql_main_user");
  } else {
    dataBase = QSqlDatabase::addDatabase("QMYSQL", "mysql_main_user");
    dataBase.setUserName("Effects");
    dataBase.setPassword("Effects");

    dataBase.setHostName(ip);
    dataBase.setPort(3306);
    dataBase.setDatabaseName("myuser");
  }
  if (!dataBase.open()) throw std::runtime_error(dataBase.lastError().text().toStdString());
//    dataBase.transaction();
  isInit = true;
}

coreSqlUser::coreSqlUser() {

}

CORE_NAMESPACE_E
