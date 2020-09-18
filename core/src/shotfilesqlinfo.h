#pragma once

#include "filesqlinfo.h"

CORE_NAMESPACE_S

class CORE_EXPORT shotFileSqlInfo :public fileSqlInfo
{

public:
    shotFileSqlInfo();
    shotFileSqlInfo(const qint64 &ID_);

    void insert() override;
    void updata() override;
    void deleteSQL() override;


    //外键查询
    episodesPtr getEpisdes();
    void setEpisdes( const episodesPtrW& eps_);

    shotPtr getShot();
    void setShot(const shotPtrW& shot_);
    const static QString SQLCreateTable;

private:
    qint64 __episodes__;
    qint64 __shot__;
    qint64 __file_class__;
    qint64 __file_type__;

    episodesPtrW eps_ptrW;
    shotPtrW shot_ptrW;

};

CORE_DNAMESPACE_E
