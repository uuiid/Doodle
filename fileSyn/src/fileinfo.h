#ifndef FILEINFO_H
#define FILEINFO_H

#include "fileSyn_global.h"
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtSql/QSqlQuery>
#include <QtCore/QSharedPointer>

DNAMESPACE_S
class FILESYN_EXPORT fileInfo
{
public:
    fileInfo();
    fileInfo(const QDir & dirRoot1, const QDir & dirRoot2, const QDir & dir);
    fileInfo(const QString & dirRoot1,const QString & dirRoot2,const QString & dir);


    static QString getCreataTableCom(const QString &tableName);

    bool subAndUpdataSQL();
    bool SQLupdata(QSqlQuery &query);
    bool SQLdelete();

private:

    bool SQLinstall(QSqlQuery &query);

    QDir path;
    QString absPath_1;
    QDateTime modifyTime_1;
    qint64 fileSize_1;

    QString absPath_2;
    QDateTime modifyTime_2;
    qint64 fileSize_2;

    QString tableName;
    static QString createTableSqlcom;
    static QString subInfo;
};

typedef QSharedPointer<fileInfo> fileInfoptr;

DNAMESPACE_E

#endif // FILEINFO_H
