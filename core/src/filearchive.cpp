#include "filearchive.h"
#include <QUrl>

CORE_NAMESPACE_S
filearchive::filearchive()
{
}

filearchive::~filearchive()
{
}

void filearchive::update(const QUrl &path)
{
    QUrl path2 = path;
    if (!isInCache(path))
    {
        path2 = copyToCache(path);
    }

    insterArchive();
    _updata(path2);
}

CORE_DNAMESPACE_E