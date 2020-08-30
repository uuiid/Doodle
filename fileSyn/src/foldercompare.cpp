#include "foldercompare.h"
#include "sqlconnect.h"
#include <QDirIterator>
DNAMESPACE_S


//folderCompare::folderCompare(QDir path1, QDir path2)
//{
//    if(!path1.exists()) path1.mkdir(path1.absolutePath());
//    if(!path2.exists()) path2.mkdir(path2.absolutePath());
//    filesynPair = std::make_pair(path1,path2);
//}

bool folderCompare::initDB()
{
    sqlConnect & sqldb = sqlConnect::GetSqlConnect();
    if (!sqldb.openSqlDB(QDir(filesynPair.first))) return false;
    return true;
}

bool folderCompareSyn::scan()
{
    if(!initDB()) return false;
    scanPath(filesynPair.first, fileInfoFirst,"First");
    scanPath(filesynPair.second,fileInfoSecond,"Second");
    return true;
}

bool folderCompareSyn::scanPath(const QDir & path,std::map<QString,fileInfoptr> & fileInfoList, const QString & tableName)
{
    QDirIterator dirIter(path.canonicalPath(),QDir::NoDotAndDotDot ,QDirIterator::Subdirectories);
    while (dirIter.hasNext()) {
        if(QFileInfo(dirIter.filePath()).isFile()){
            // 创建文件信息列表
            fileInfoptr info(new fileInfo(dirIter.filePath(),tableName));
            fileInfoList.insert(std::make_pair(dirIter.filePath(),info));
            //前进
            dirIter.next();
        }
    }
    return true;
}

bool folderCompareSyn::subFileInfo(const std::map<QString, fileInfoptr> &fileInfoList)
{
    return true;
}

DNAMESPACE_E
