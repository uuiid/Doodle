#include "coresqldata.h"



CORE_NAMESPACE_S

coresqldata::coresqldata()
{
    idP = -1;
}

qint64 coresqldata::getIdP() const
{
    return idP;
}

bool coresqldata::isNULL() const
{
    if(idP > 0){
        return true;
    }else {
        return false;
    }
}

CORE_DNAMESPACE_E
