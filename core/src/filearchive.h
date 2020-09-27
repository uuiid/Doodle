#pragma once

#include "core_global.h"

CORE_NAMESPACE_S

class filearchive
{
public:
    filearchive();
    virtual ~filearchive();

    void update(const QUrl &path);
    QUrl down(const QUrl &path);

protected:
    virtual const QUrl copyToCache(const QUrl &path) const = 0;
    virtual bool isInCache(const QUrl &path) const = 0;
    virtual void insterArchive() const = 0;
    virtual void _updata(const QUrl & path);
};

CORE_DNAMESPACE_E