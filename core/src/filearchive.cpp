#include "filearchive.h"
#include <QFileInfo>
#include <QVector>

CORE_NAMESPACE_S

void fileArchive::update(const QFileInfo &path)
{
    p_soureFile.clear();
    p_soureFile.append(path);
    _generateFilePath();
    if (!isInCache())
    {
        copyToCache();
    }
    _updata();
    insertArchive();
}

void fileArchive::update(const QfileInfoVector & filelist) 
{
    if(filelist.size() == 1) update(filelist[0]);
}

QfileInfoVector fileArchive::down(const QFileInfo &path) 
{
    return QfileInfoVector();
}

CORE_NAMESPACE_E