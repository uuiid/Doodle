#pragma once



#include "fileSyn_global.h"
#include "fileinfo.h"
#include <QtCore/QDir>
#include <QtSql/QSqlTableModel>
#include <map>

SYN_NAMESPACE_S
typedef std::pair<QDir,QDir> synPair;
typedef std::map<QString,fileInfoptr> synMap;
typedef std::map<QString,std::pair<fileInfoptr,fileInfoptr>> synMapPair;

class FILESYN_EXPORT folderCompare
{
public:
//    folderCompare() {};
    folderCompare(const QDir path1,const QDir path2) :filesynPair(path1,path2) {};
    virtual bool scan() = 0;
protected:
    virtual bool scanPath(const QDir & path, synMap &filemap) = 0;
    virtual bool subFileInfo(const synMapPair & fileInfoList) = 0;
    synPair filesynPair;
    synMap fileInfoFirst;
    synMap fileInfoSecond;
    synMapPair filemap;

    QSharedPointer<QSqlTableModel> table;

    bool initDB();

};

class FILESYN_EXPORT folderCompareSyn :public folderCompare
{
public:
    using folderCompare::folderCompare;
    bool scan() override;
protected:
    bool scanPath(const QDir & path, synMap &filemap) override;
    bool subFileInfo(const synMapPair &fileInfoList) override;
};


DNAMESPACE_E

