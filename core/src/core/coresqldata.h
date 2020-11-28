#pragma once

#include "core_global.h"

CORE_NAMESPACE_S

 class CORE_API coresqldata
{
public:
    coresqldata();

    virtual void insert() = 0;
    virtual void updateSQL() = 0;
    virtual void deleteSQL() = 0;

    [[nodiscard]] int64_t getIdP() const;

    [[nodiscard]] bool isNULL() const;
    [[nodiscard]] inline bool isInsert() const;
protected:
    qint64 idP;
};

bool coresqldata::isInsert() const { return !isNULL();}
CORE_NAMESPACE_E
