#pragma once

#include "core_global.h"

CORE_NAMESPACE_S

class CORE_EXPORT coresqldata
{
public:
    coresqldata();

    virtual void insert() = 0;
    virtual void updateSQL() = 0;
    virtual void deleteSQL() = 0;

    qint64 getIdP() const;

    bool isNULL() const;
    bool isInsert() const { return !isNULL();};
protected:
    qint64 idP;
};

CORE_NAMESPACE_E
