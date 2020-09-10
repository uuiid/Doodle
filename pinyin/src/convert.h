#ifndef CONVERT_H
#define CONVERT_H

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
    bool search();
    QString toEnOne(const QString &conStr);

private:
    QSqlDatabase dataBase;
    QSharedPointer<QDir> sqlDBPath;
    bool isinitDB;
    QSharedPointer<QRegularExpression> re;
    QSharedPointer<QSqlQuery> query;
};


DNAMESPACE_E
#endif // CONVERT_H
