#ifndef FILEINFO_H
#define FILEINFO_H

#include "fileSyn_global.h"
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtSql/QSqlQuery>
#include <QtCore/QSharedPointer>
#include "d_setting.h"

SYN_NAMESPACE_S
class FILESYN_EXPORT fileInfo
{
public:
    fileInfo();
    fileInfo(const QDir & dirRoot1, const QDir & dirRoot2, const QDir & dir);
    fileInfo(const QString & dirRoot1,const QString & dirRoot2,const QString & dir);


    static QString getCreataTableCom(const QString &tableName);
    //提交和更新
    bool subAndUpdataSQL();
    //删除
    bool SQLdelete();
    //比较并进行同步
    bool com();
    bool syn();

protected:

    bool SQLupdata(QSqlQuery &query);
    bool SQLinstall(QSqlQuery &query);
    bool SQLSelect();

    bool syn_(const d_setting::synSet &set);

    virtual bool down();
    virtual bool upload();
    virtual bool del();

    d_setting::synSet synset;

    QDir path;
    QString absPath_1;
    QDateTime modifyTime_1;
    qint64 fileSize_1;

    QString absPath_2;
    QDateTime modifyTime_2;
    qint64 fileSize_2;

    QDateTime syn_time;
    QString tableName;
    static QString createTableSqlcom;

};

typedef QSharedPointer<fileInfo> fileInfoptr;

DNAMESPACE_E

#endif // FILEINFO_H
