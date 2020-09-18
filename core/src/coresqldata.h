#pragma once

#include "core_global.h"


CORE_NAMESPACE_S

class coresqldata
{
public:
    coresqldata();

    virtual void insert() = 0;
    virtual void updata() = 0;
    virtual void deleteSQL() = 0;

    qint64 getIdP() const;

    bool isNULL() const;
protected:
    qint64 idP;
};

CORE_DNAMESPACE_E
