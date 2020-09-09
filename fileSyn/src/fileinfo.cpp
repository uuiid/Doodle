#include "fileinfo.h"
#include <QtCore/QFileInfo>
#include <QtCore/QVariant>
#include <QtSql/QSqlError>
#include <QtDebug>
#include <QFile>

SYN_NAMESPACE_S

QString fileInfo::createTableSqlcom = "CREATE TABLE IF NOT EXISTS `%1` (\
        file_size_1 integer,\
        file_size_2 integer,\
        modify_time_1 text,\
        modify_time_2 text,\
        syn_time text,\
        path text primary key\
        )";

fileInfo::fileInfo()
{

}

fileInfo::fileInfo(const QString &dirRoot1, const QString &dirRoot2, const QString &dir)
{
    QString root1 = QString("%1/%2").arg(dirRoot1, dir);
    absPath_1 = root1;
    path = QDir(dir);
    if (QFile(root1).exists()){
        QFileInfo info(root1);
        fileSize_1 =info.size();
        modifyTime_1 = info.lastModified();
    }else {
        fileSize_1 = 0;
        modifyTime_1 = QDateTime();
    }

    QString root2 = QString("%1/%2").arg(dirRoot2, dir);
    absPath_2 = root2;
    if (QFile(root2).exists()){
        QFileInfo info(root2);
        fileSize_2 =info.size();
        modifyTime_2 = info.lastModified();
    }else {
        fileSize_2 = 0;
        modifyTime_2 = QDateTime();
    }

}

fileInfo::fileInfo(const QDir & dirRoot1,const QDir & dirRoot2, const QDir & dir)
{
    QString root1 = QString("%1/%2").arg(dirRoot1.path(),dirRoot1.path());
    path = dir;
    absPath_1 = root1;
    if (QFile(root1).exists()){
        QFileInfo info(root1);
        fileSize_1 =info.size();
        modifyTime_1 = info.lastModified();
    }else {
        fileSize_1 = 0;
        modifyTime_1 = QDateTime();
    }

    QString root2 = QString("%1/%2").arg(dirRoot2.path(),dirRoot1.path());
    absPath_2 = root2;
    if (QFile(root2).exists()){
        QFileInfo info(root2);
        fileSize_2 =info.size();
        modifyTime_2 = info.lastModified();
    }else {
        fileSize_2 = 0;
        modifyTime_2 = QDateTime();
    }
}

QString fileInfo::getCreataTableCom(const QString &tableName_)
{
    return createTableSqlcom.arg(tableName_);
}

bool fileInfo::subAndUpdataSQL()
{
    QSqlQuery query(nullptr,QSqlDatabase::database("sqlite_syn_db",true));
    if(SQLSelect()){
        if(!SQLupdata(query)) {qDebug() << query.lastError().text();return false;}//更新数据库数据
        return true;
    }else {
        if(!SQLinstall(query)) {qDebug() << query.lastError().text();return false;}//没有时提交数据库数据
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
    return query.exec(sql.arg("file",path.path()));

}

bool fileInfo::com()
{
    d_setting& set = d_setting::GetSetting();
    if(fileSize_1 == fileSize_2 && modifyTime_1 == modifyTime_2){
        synset = d_setting::synSet::ignore;
        return true;
    }else if (modifyTime_1 > modifyTime_2) {
        if(!syn_time.isNull()){
            if(fileSize_2 !=0 && modifyTime_2 > syn_time){
                synset = set.getSynDef_conflict();
            }else if(fileSize_2 !=0 && modifyTime_2 < syn_time) {
                synset = set.getSynDef_local_new();
            }else {
                synset = set.getSynDef_local_only();
            }
        }else {
            if(fileSize_2 !=0){
                synset = set.getSynDef_local_new();
            }else {
                synset = set.getSynDef_local_only();
            }
        }
    }else if (modifyTime_1 < modifyTime_2) {
        if(!syn_time.isNull()){
            if(fileSize_1 !=0 && modifyTime_1 > syn_time){
                synset = set.getSynDef_conflict();
            }else if(fileSize_1 !=0 && modifyTime_1 < syn_time) {
                synset = set.getSynDef_server_new();
            }else {
                synset = set.getSynDef_server_only();
            }
        }else {
            if(fileSize_1 !=0){
                synset = set.getSynDef_server_new();
            }else {
                synset = set.getSynDef_server_only();
            }
        }
    }
    return true;
}

bool fileInfo::syn()
{
    return syn_(synset);
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

bool fileInfo::SQLSelect()
{
    QSqlQuery query(nullptr,QSqlDatabase::database("sqlite_syn_db",true));
    QString sql = "SELECT file_size_1,file_size_2,modify_time_1,modify_time_2,syn_time FROM `%1` WHERE path='%2'";
    query.exec(sql.arg("file",path.path()));
    if(query.next()){
        syn_time = query.value(4).toDateTime();
//        modifyTime_1 = query.value(2).toDateTime();
//        modifyTime_2 = query.value(3).toDateTime();
        return true;
    }else {
        syn_time = QDateTime();
        return false;
    }
}

bool fileInfo::syn_(const d_setting::synSet & set)
{
    switch (set) {
    case d_setting::synSet::down: return down();
    case d_setting::synSet::updata: return upload();
    case d_setting::synSet::ignore : return true;
    case d_setting::synSet::del: return del();
    }
    return true;
}

bool fileInfo::down()
{
    d_setting& set = d_setting::GetSetting();
    QFile path1(absPath_1);
    if(path1.exists()){
        path1.rename(set.getBackup(path).toLocalFile());
    }
    return QFile(absPath_2).copy(absPath_1);
}

bool fileInfo::upload()
{
    d_setting& set = d_setting::GetSetting();
    QFile path1(absPath_2);
    if(path1.exists()){
        if(!path1.rename(set.getBackup(path).toLocalFile())) return false;
    }
    return QFile(absPath_1).copy(absPath_2);
}

bool fileInfo::del()
{
    d_setting& set = d_setting::GetSetting();
    QFile path1(absPath_1);
    if(path1.exists()){
        path1.remove(set.getBackup(path).toLocalFile());
    }
    QFile path2(absPath_2);
    if(path2.exists()){
        path2.remove(set.getBackup(path).toLocalFile());
    }
    return true;
}

DNAMESPACE_E
