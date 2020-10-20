#pragma once

#include "core_global.h"
#include "filearchive.h"

CORE_NAMESPACE_S

class CORE_EXPORT fileArchiveOneFile : public fileArchive
{

public:
    fileArchiveOneFile() : p_fileInfo(nullptr),
                           p_cacheFilePath(),
                           p_Path(){};
    virtual ~fileArchiveOneFile(){};

protected:
    //复制文件到缓存目录中
    virtual void copyToCache() const override;
    //是否存在缓存目录中
    virtual bool isInCache() override;
    virtual void insertArchive() override;
    virtual void _updata() override;

protected:
    //主要属性  必须进行初始化
    fileSqlInfoPtr p_fileInfo;

    QFileInfo p_cacheFilePath;
    QString p_Path;
};

CORE_NAMESPACE_E