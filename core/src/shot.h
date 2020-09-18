#pragma once

#include "core_global.h"

CORE_NAMESPACE_S

class CORE_EXPORT shot
{
public:
    shot();
    shot(const qint64 &ID_);

    qint64 getIdP() const;

private:
    qint64 idP;
};

CORE_DNAMESPACE_E
