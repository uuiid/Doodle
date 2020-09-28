#pragma once

#include "core_global.h"
// #include <QFileInfo>
// #include <QVector>

CORE_NAMESPACE_S

class CORE_EXPORT fileArchive
{
public:
    fileArchive():p_soureFile(){};
    virtual ~fileArchive(){};

    virtual void update(const QFileInfo &path);
    virtual void update(const QfileInfoVector & filelist);
    virtual QfileInfoVector down(const QFileInfo &path);

protected:
    //复制到和缓存文件夹
    virtual void copyToCache() const = 0;
    //判断是否在缓存文件夹
    virtual bool isInCache() = 0;
    //提交到数据库
    virtual void insertArchive() = 0;
    //上传文件
    virtual void _updata() = 0;
    //组合出服务器路径路径
    virtual void _generateFilePath() = 0;

protected:
    QfileInfoVector p_soureFile;
};

CORE_DNAMESPACE_E