#include "fileinfo.h"
#include <QtCore/QFileInfo>
#include <QtCore/QVariant>
#include <QtSql/QSqlError>
#include <QtDebug>
DNAMESPACE_S

QString fileInfo::createTableSqlcom = "CREATE TABLE IF NOT EXISTS `%1` (\
        file_size_1 integer,\
        file_size_2 integer,\
        modify_time_1 text,\
        modify_time_2 text,\
        path text primary key\
        )";

QString fileInfo::subInfo = "INSERT INTO `%1`(file_size,modify_time,path) \
VALUES (:file_size,:modify_time,:path)";

fileInfo::fileInfo()
{

}

fileInfo::fileInfo(const QString &dirRoot1, const QString &dirRoot2, const QString &dir)
{
    QDir root1 = dirRoot1;
    root1.cd(dir);
    path = QDir(dir);
    if (root1.exists()){
        QFileInfo info(root1.absolutePath());
        absPath_1 = root1.absolutePath();
        fileSize_1 =info.size();
        modifyTime_1 = info.lastModified();
    }else {
        absPath_1 = "";
        fileSize_1 = 0;
        modifyTime_1 = QDateTime();
    }

    QDir root2 = dirRoot2;
    root2.cd(dir);
    if (root2.exists()){
        QFileInfo info(root2.absolutePath());
        absPath_1 = root2.absolutePath();
        fileSize_1 =info.size();
        modifyTime_1 = info.lastModified();
    }else {
        absPath_1 = "";
        fileSize_1 = 0;
        modifyTime_1 = QDateTime();
    }

}

fileInfo::fileInfo(const QDir & dirRoot1,const QDir & dirRoot2, const QDir & dir)
{
    QDir root1 = dirRoot1;
    root1.cd(dir.path());
    path = dir;
    if (root1.exists()){
        QFileInfo info(root1.absolutePath());
        absPath_1 = root1.absolutePath();
        fileSize_1 =info.size();
        modifyTime_1 = info.lastModified();
    }else {
        absPath_1 = "";
        fileSize_1 = 0;
        modifyTime_1 = QDateTime();
    }

    QDir root2 = dirRoot2;
    root2.cd(dir.path());
    if (root2.exists()){
        QFileInfo info(root2.absolutePath());
        absPath_1 = root2.absolutePath();
        fileSize_1 =info.size();
        modifyTime_1 = info.lastModified();
    }else {
        absPath_1 = "";
        fileSize_1 = 0;
        modifyTime_1 = QDateTime();
    }
}

QString fileInfo::getCreataTableCom(const QString &tableName_)
{
    return createTableSqlcom.arg(tableName_);
}

bool fileInfo::subAndUpdataSQL()
{
    QSqlQuery query(nullptr,QSqlDatabase::database("sqlite_syn_db",true));
    QString sql = "SELECT * FROM `%1` WHERE path='%2'";
    query.exec(sql.arg("file",path.path()));
    if(query.next()){
        if(!SQLupdata(query)) {qDebug() << query.lastError().text();return false;}
        return true;
    }else {
        if(!SQLinstall(query)) {qDebug() << query.lastError().text();return false;}
        return true;
    }
}

bool fileInfo::SQLupdata(QSqlQuery & query)
{
    QString sql= "UPDATE `%1` SET file_size_1 =:fs1, file_size_2=:fs2,modify_time_1=:mo1,modify_time_2=:mo2 WHERE path =:p";
    query.prepare(sql.arg("file"));
    query.bindValue(":fs1",fileSize_1);
    query.bindValue(":fs2",fileSize_2);
    query.bindValue(":mo1",modifyTime_1);
    query.bindValue(":mo2",modifyTime_2);
    query.bindValue(":p",path.path());
    return query.exec();
}

bool fileInfo::SQLdelete()
{
    QString sql = "DELETE FROM %1 WHERE path='%2'";
    QSqlQuery query(nullptr,QSqlDatabase::database("sqlite_syn_db",true));
    return query.exec(sql.arg("file"));

}
bool fileInfo::SQLinstall(QSqlQuery & query)
{
    QString sql= "INSERT INTO `%1`(file_size_1, file_size_2, modify_time_1, modify_time_2, path) VALUES (:fs1,:fs2,:mo1,:mo2,:p)";
    query.prepare(sql.arg("file"));
    query.bindValue(":fs1",fileSize_1);
    query.bindValue(":fs2",fileSize_2);
    query.bindValue(":mo1",modifyTime_1);
    query.bindValue(":mo2",modifyTime_2);
    query.bindValue(":p",path.path());
    return query.exec();
}

DNAMESPACE_E
