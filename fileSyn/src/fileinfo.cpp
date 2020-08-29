#include "fileinfo.h"
#include <QtCore/QFileInfo>
#include <QtCore/QVariant>

DNAMESPACE_S

QString fileInfo::createTableSqlcom = "CREATE TABLE IF NOT EXISTS `%1` (\
        file_size integer,\
        modify_time text,\
        path text primary key\
        )";

QString fileInfo::subInfo = "INSERT INTO `%1`(file_size,modify_time,path) \
VALUES (:file_size,:modify_time,:path)";

fileInfo::fileInfo()
{

}

fileInfo::fileInfo(const QString &dir,const QString & tableName_)
{
    QDir dir_(dir);
    if (dir_.exists()){
        QFileInfo info(dir_.absolutePath());
        absPath = dir_.absolutePath();
        path = dir_;
        fileSize =info.size();
        modifyTime = info.lastModified();
    }else {
        absPath = "";
        path = QDir();
        fileSize = 0;
        modifyTime = QDateTime();
    }
    tableName = tableName_;

}

QSqlQuery fileInfo::subFileInfo(QSqlQuery &bindSql)
{
    bindSql.bindValue(":file_size",   QVariant(int(fileSize)));
    bindSql.bindValue(":modify_time", QVariant(modifyTime));
    bindSql.bindValue(":path",        QVariant(path.canonicalPath()));
    return bindSql;
}

fileInfo::fileInfo(const QDir &dir, const QString & tableName_)
{
    if (dir.exists()){
        QFileInfo info(dir.absolutePath());
        absPath = dir.absolutePath();
        path = dir;
        fileSize =info.size();
        modifyTime = info.lastModified();
    }else {
        absPath = "";
        path = QDir();
        fileSize = 0;
        modifyTime = QDateTime();
    }
    tableName = tableName_;

}

QString fileInfo::getSqlCom()
{
    return subInfo.arg(tableName);
}

QString fileInfo::getCreataTableCom(const QString &tableName_)
{
    return createTableSqlcom.arg(tableName_);
}

DNAMESPACE_E
