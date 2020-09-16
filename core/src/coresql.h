#pragma once


#include "core_global.h"
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSharedPointer>

CORE_NAMESPACE_S

/*
* 数据库连接类
* 全局静态类
*/
class CORE_EXPORT coreSql : public QObject
{
    Q_OBJECT
public:
    static coreSql& getCoreSql();
    coreSql & operator =(const coreSql & s) = delete ;
    coreSql(const coreSql& s) =delete;

    ~coreSql();
    sqlQuertPtr getquery();
    void closeDataBase();
    bool commitDataBase();
public slots:
    void initDB(const QString& ip,const QString &dataName);

private:
    coreSql();

private:
    bool isInit = false;

};


/*
数据库中的user类 只有在注册的时候会用到
*/
class CORE_EXPORT coreSqlUser :public QObject
{
    Q_OBJECT
public:
    static coreSqlUser& getCoreSqlUser();
    coreSqlUser & operator =(const coreSqlUser & s) = delete ;
    coreSqlUser(const coreSqlUser& s) =delete;

    mapStringPtr getUser();

public slots:
    void initDB(const QString &ip,const QString &dataName);
private:
    coreSqlUser();

private:
    bool isInit = false;

};

CORE_DNAMESPACE_E

