#ifndef FOLDERCOMPARE_H
#define FOLDERCOMPARE_H


#include "fileSyn_global.h"
#include "fileinfo.h"
#include <QtCore/QDir>
#include <map>

DNAMESPACE_S
typedef std::pair<QDir,QDir> synPair;

class FILESYN_EXPORT folderCompare
{
public:
    folderCompare();
    folderCompare(QDir path1,QDir path2);
    virtual bool scan() = 0;
protected:
    virtual bool scanPath(const QDir & path,std::map<QString,fileInfoptr> & fileInfoList, const QString & tableName) = 0;
    virtual bool subFileInfo(const std::map<QString,fileInfoptr> & fileInfoList) = 0;
    synPair filesynPair;
    std::map<QString,fileInfoptr> fileInfoFirst;
    std::map<QString,fileInfoptr> fileInfoSecond;
    bool initDB();

};

class FILESYN_EXPORT folderCompareSyn :public folderCompare
{
public:
    bool scan() override;
protected:
    bool scanPath(const QDir & path, std::map<QString,fileInfoptr> & fileInfoList, const QString &tableName) override;
    bool subFileInfo(const std::map<QString, fileInfoptr> &fileInfoList) override;
};


DNAMESPACE_E
#endif // FOLDERCOMPARE_H
