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
    fileInfo(const QDir & dir,   const QString & tableName_);
    fileInfo(const QString & dir,const QString & tableName_);

    QString subFileInfo(QSqlQuery & bindSql);



    static QString getCreataTableCom(const QString &tableName);


private:

    QDir path;
    QString absPath;

    QDateTime modifyTime;

    qint64 fileSize;

    QString tableName;
    static QString createTableSqlcom;
    static QString subInfo;
};

typedef QSharedPointer<fileInfo> fileInfoptr;

DNAMESPACE_E

#endif // FILEINFO_H
