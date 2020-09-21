#pragma once

#include "core_global.h"
#include "coresqldata.h"

CORE_NAMESPACE_S

class CORE_EXPORT episodes :public coresqldata
{
public:
    episodes();
    episodes(const qint64 & ID_);

    void insert() override;
    void updateSQL() override;
    void deleteSQL() override;


    qint64 getEpisdes() const;
    void setEpisdes(const qint64 &value);

private:
    qint64 episP;

};

CORE_DNAMESPACE_E
