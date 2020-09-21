#pragma once

#include "core_global.h"

#include "coresqldata.h"


CORE_NAMESPACE_S


class CORE_EXPORT fileClass :public coresqldata
{
public:
    fileClass();
    fileClass(const qint64 & ID_);

    void insert() override;
    void updateSQL() override;
    void deleteSQL() override;


    QString getFilecalssP() const;
    void setFilecalssP(const QString &value);

    episodesPtr getEps_ptrW();
    void setEps_ptrW(const episodesPtrW &value);

    shotPtr getShot_ptrW();
    void setShot_ptrW(const shotPtrW &value);

    qint64 getIdP();

private:
    QString filecalssP;

    episodesPtrW eps_ptrW;
    shotPtrW shot_ptrW;
    
    qint64 __shot__;
    qint64 __eps__;
};

CORE_DNAMESPACE_E
