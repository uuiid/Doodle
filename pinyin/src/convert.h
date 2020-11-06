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
    std::string toEn(const std::string &conStr);

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

