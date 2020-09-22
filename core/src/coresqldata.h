#pragma once

#include "core_global.h"

CORE_NAMESPACE_S

class coresqldata
{
public:
    coresqldata();

    virtual void insert() = 0;
    virtual void updateSQL() = 0;
    virtual void deleteSQL() = 0;

    qint64 getIdP() const;
    qint64 getIdP(const bool & useInsert);

    bool isNULL() const;
protected:
    qint64 idP;

    void getInsertID(sqlQuertPtr &query);
};

CORE_DNAMESPACE_E
