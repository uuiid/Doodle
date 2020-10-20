#include "coresql.h"
#include <QSqlError>
#include <QVariant>
#include <QMap>
#include <QtDebug>


CORE_NAMESPACE_S
coreSql::coreSql()
{

}

void coreSql::initDB(const QString &ip,const QString &dataName)
{
    QSqlDatabase dataBase;
    if(QSqlDatabase::contains("mysql_main_data")){
        dataBase = QSqlDatabase::database("mysql_main_data");
    }else {
        dataBase = QSqlDatabase::addDatabase("QMYSQL","mysql_main_data");
        dataBase.setUserName("Effects");
        dataBase.setPassword("Effects");

        dataBase.setHostName(ip);
        dataBase.setPort(3306);
        dataBase.setDatabaseName(dataName);
    }
    if(!dataBase.open()) throw std::runtime_error(dataBase.lastError().text().toStdString());
//    dataBase.transaction();
    isInit = true;
}

coreSql::~coreSql()
{
    closeDataBase();
}

coreSql &coreSql::getCoreSql()
{
    static coreSql install;
    return install;
}

sqlQuertPtr coreSql::getquery()
{
    QSqlDatabase dataBase = QSqlDatabase::database("mysql_main_data");
    dataBase.open();
//    dataBase.transaction();
    if(!isInit) throw std::runtime_error("not init_ DB (sql mian data)");
    sqlQuertPtr queryPtr(new QSqlQuery(dataBase));
    return queryPtr;
}

void coreSql::closeDataBase()
{
    QSqlDatabase::database("mysql_main_data").close();
}

bool coreSql::commitDataBase()
{
    QSqlDatabase dataBase = QSqlDatabase::database("mysql_main_data");
    dataBase.open();
    return dataBase.commit();
}

coreSqlUser &coreSqlUser::getCoreSqlUser()
{
    static coreSqlUser install;
    return install;
}

mapStringPtr coreSqlUser::getUser()
{
    QSqlDatabase dataBase = QSqlDatabase::database("mysql_main_user");
    dataBase.open();
    if(!isInit) throw std::runtime_error("not init_ DB (sql mian data)");
    QSqlQuery query(dataBase);

    query.exec("SELECT user,password FROM user;");
    mapStringPtr list;
    while (query.next()) {
        QString user = query.value(0).toString();
        QString powr = query.value(1).toString();
        list.insert(user,powr);
    }
    return list;
}

void coreSqlUser::initDB(const QString& ip,const QString &dataName)
{
    QSqlDatabase dataBase;
    if(QSqlDatabase::contains("mysql_main_user")){
        dataBase = QSqlDatabase::database("mysql_main_user");
    }else {
        dataBase = QSqlDatabase::addDatabase("QMYSQL","mysql_main_user");
        dataBase.setUserName("Effects");
        dataBase.setPassword("Effects");

        dataBase.setHostName(ip);
        dataBase.setPort(3306);
        qDebug()<<dataName;
        dataBase.setDatabaseName("myuser");
    }
    if(!dataBase.open()) throw std::runtime_error(dataBase.lastError().text().toStdString());
//    dataBase.transaction();
    isInit = true;
}

coreSqlUser::coreSqlUser()
{

}

CORE_NAMESPACE_E
