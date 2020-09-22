#include "coresqldata.h"

#include <QSqlQuery>
#include <QVariant>

CORE_NAMESPACE_S

coresqldata::coresqldata()
{
    idP = -1;
}

qint64 coresqldata::getIdP() const
{
    if (idP >= 0)
    {
        return idP;
    }
    else
    {
        throw std::runtime_error("not insert db so not id");
    }
}

qint64 coresqldata::getIdP(const bool &useInsert)
{
    if (idP >= 0)
    {
        return idP;
    }
    else
    {
        if (useInsert)
        {
            insert();
            return idP;
        }
        else
        {
            throw std::runtime_error("not insert db so not id");
        }
    }
}

bool coresqldata::isNULL() const
{
    if (idP >= 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void coresqldata::getInsertID(sqlQuertPtr &query)
{
    if (!query->exec("SELECT LAST_INSERT_ID() as id_;"))
        return;
    if (query->next())
    {
        idP = query->value("id_").toInt();
    }
}
CORE_DNAMESPACE_E
