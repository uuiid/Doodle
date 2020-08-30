#include "sqlconnect.h"
#include <QDebug>
#include <QSqlQuery>
#include <QCoreApplication>
DNAMESPACE_S

sqlConnect::sqlConnect()
{
//    dataBase = sqldatabasePtr(new QSqlDatabase());
}

sqlConnect::~sqlConnect()
{

}

sqlConnect& sqlConnect::GetSqlConnect()
{
    static sqlConnect instance;
    return instance;
}

bool sqlConnect::openSqlDB(const QDir & dir)
{
//    QSqlDatabase dataBase;
    if(!setRoot(dir)) return false;
    if (QSqlDatabase::contains("sqlite_syn_db"))
    {
        return true;
    }
    else
    {
        dataBase = QSqlDatabase::addDatabase("QSQLITE","sqlite_syn_db");
        dataBase.setDatabaseName(Root.canonicalPath() + "/doodle_syn.dol_db");
        if(dataBase.open())
        {
            qDebug("open db");
            return true;
        }
    }
    return false;
}

bool sqlConnect::setRoot(const QDir &dir)
{
    if(dir.exists()){
        Root = dir;
        return true;
    }else{
        if (!dir.mkdir(".")) return false;
        Root = dir;
        return true;
    }
    return false;
}

bool sqlConnect::createTable(const QString &sqlcom)
{
    QSqlQuery sql_query(dataBase);
    sql_query.prepare(sqlcom);
    if (!sql_query.exec()){
        qDebug()  << "not exe ->" << sqlcom;
        return false;
    }
    dataBase.commit();
    return true;
}

void sqlConnect::closedb()
{
    dataBase.close();
    dataBase = QSqlDatabase();
    dataBase.removeDatabase("sqlite_syn_db");
}

DNAMESPACE_E
