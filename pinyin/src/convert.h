#ifndef CONVERT_H
#define CONVERT_H

#include "pinyin_global.h"
#include <QSqlDatabase>
#include <QSharedPointer>
#include <QRegularExpression>
#include <QSqlQuery>

PINYIN_NAMESPACE_S

class PINYIN_EXPORT convert
{
public:
    convert();
    QString toEn(const QString &conStr);
private:
    void initDB();
    void initQuery();
    void initExp();
    QString toEnOne(const QString &conStr);

private:
    QSqlDatabase dataBase;
    bool isinitDB;
    QSharedPointer<QRegularExpression> re;
    QSharedPointer<QSqlQuery> query;
};


DNAMESPACE_E
#endif // CONVERT_H
