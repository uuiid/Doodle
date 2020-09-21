#pragma once

#include "core_global.h"
#include "coresqldata.h"


CORE_NAMESPACE_S

class CORE_EXPORT shot :public coresqldata
{
public:

    enum class e_shotAB{
        _ = 0,
        B = 1,
        C = 2,
        D = 3,
        E = 4,
        F = 5,
        G = 6,
        H = 7
    };

    shot();
    shot(const qint64 &ID_);

    void insert() override;
    void updateSQL() override;
    void deleteSQL() override;

    void setEpisdes(const episodesPtrW &value);
    episodesPtr getEpisdes();

    void setShot(const qint64 &sh, const e_shotAB &ab = e_shotAB::_);

private:
    qint64 p_qint_shot_;
    e_shotAB p_qenm_shotab;

    episodesPtrW p_ptr_eps;
    qint64 __episodes__;

};

CORE_DNAMESPACE_E
