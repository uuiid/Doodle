#pragma once


#include "pinyin_global.h"
#include <QSqlDatabase>
#include <QSharedPointer>
#include <QRegularExpression>
#include <QSqlQuery>
#include <QDir>

PINYIN_NAMESPACE_S

class PINYIN_EXPORT convert
{
public:
    convert();
    ~convert();
    QString toEn(const QString &conStr);
private:
    void initDB();
    void initQuery();
    void initExp();
    bool createDB();
    QString toEnOne(const QString &conStr);

private:
    QSqlDatabase dataBase;
    QSharedPointer<QDir> sqlDBPath;
    QSharedPointer<QRegularExpression> re;
    QSharedPointer<QSqlQuery> query;
    
    bool isinitDB;
    static QTemporaryFile tmpDBFile;
};


DNAMESPACE_E

