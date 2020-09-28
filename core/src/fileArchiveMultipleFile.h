#pragma once

#include "core_global.h"
#include "fileArchiveOneFile.h"

CORE_NAMESPACE_S

class fileArchiveMultipleFile : public fileArchiveOneFile
{
private:
    
public:
    fileArchiveMultipleFile(){};
    virtual ~fileArchiveMultipleFile(){};

    virtual void update(const QfileInfoVector & filelist) override;
    virtual QfileInfoVector down(const QFileInfo & path) override;

protected:
    virtual void copyToCache() const override;
    virtual bool isInCache() override;
    virtual void insertArchive() override;
    virtual void _updata() override; 
};



CORE_DNAMESPACE_E