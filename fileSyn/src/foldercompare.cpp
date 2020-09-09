#include "foldercompare.h"
#include "sqlconnect.h"
#include <QDirIterator>
#include <QSqlTableModel>
#include <QtDebug>
#include <set>
SYN_NAMESPACE_S


//folderCompare::folderCompare(QDir path1, QDir path2)
//{
//    if(!path1.exists()) path1.mkdir(path1.absolutePath());
//    if(!path2.exists()) path2.mkdir(path2.absolutePath());
//    filesynPair = std::make_pair(path1,path2);
//}

bool folderCompare::initDB()
{
    sqlConnect & sqldb = sqlConnect::GetSqlConnect();
    if(!sqldb.openSqlDB(QDir(filesynPair.first))) return false;
    if(!sqldb.createTable(fileInfo::getCreataTableCom("file"))) return false;
    //if(!sqldb.createTable(fileInfo::getCreataTableCom("Second"))) return false;
    table = QSharedPointer<QSqlTableModel>(nullptr,sqldb.GetdataBase());
    table->setTable("file");
    table->setEditStrategy(QSqlTableModel::OnManualSubmit);
    return true;
}

bool folderCompareSyn::scan()
{
    if(!initDB()) return false;
    scanPath(filesynPair.first, fileInfoFirst);
    scanPath(filesynPair.second,fileInfoSecond);
    std::set<QString> tmplist;
    for(synMap::const_iterator iter = fileInfoFirst.begin();iter != fileInfoFirst.end();++iter){
        tmplist.insert(iter->first);
    }
    for(synMap::const_iterator iter = fileInfoSecond.begin();iter != fileInfoSecond.end();++iter){
        tmplist.insert(iter->first);
    }
    for(std::set<QString>::const_iterator iter = tmplist.begin();iter !=tmplist.end();++iter){
        if(fileInfoFirst.find(*iter) != fileInfoFirst.end()){
            filemap.insert({*iter,std::make_pair(fileInfoFirst[*iter],nullptr)});
        }else {
            filemap.insert({*iter,std::make_pair(fileInfoptr(new fileInfo()),nullptr)});
        }
        if(fileInfoSecond.find(*iter) != fileInfoSecond.end()){
            filemap[*iter].second = fileInfoSecond[*iter];
        }else {
            filemap[*iter].second = fileInfoptr(new fileInfo());
        }
    }

    subFileInfo(filemap);
    return true;
}

bool folderCompareSyn::scanPath(const QDir & path, synMap & filemap)
{
    QDirIterator dirIter(path.canonicalPath(),QDir::AllEntries|QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
    while (dirIter.hasNext()) {
        if(dirIter.fileInfo().isFile()){
            // 创建文件信息列表
//            QDir repath = path.relativeFilePath(dirIter.filePath());
//            fileInfoptr info(new fileInfo(repath));
//            filemap.insert({repath.path(),info});
            //前进
        }
        dirIter.next();
    }
    return true;
}

bool folderCompareSyn::subFileInfo(const synMapPair &fileInfoList)
{
    sqlConnect & sqlab = sqlConnect::GetSqlConnect();
    QSqlQuery query(sqlab.GetdataBase());
//    for(synMapPair::const_iterator iter = fileInfoList.begin();iter != fileInfoList.end();++iter){
//        query.exec(iter->second.first->getSQLsubobj().arg(iter->second.first.))
//    }
    sqlab.GetdataBase().commit();
    return true;
}

DNAMESPACE_E
