#pragma once

#include "filesqlinfo.h"

CORE_NAMESPACE_S

class CORE_EXPORT shotFileSqlInfo : public fileSqlInfo
{

public:
    shotFileSqlInfo();
    shotFileSqlInfo(const qint64 &ID_);

    void insert() override;
    void updateSQL() override;
    void deleteSQL() override;

    //外键查询
    episodesPtr getEpisdes();
    void setEpisdes(const episodesPtrW &eps_);

    shotPtr getShot();
    void setShot(const shotPtrW &shot_);

    fileClassPtr getFileclass();
    void setFileclass(const fileClassPtrW &value);

    fileTypePtr getFileType();
    void setFileType(const fileTypePtrW &fileType_);

private:
    qint64 __episodes__;
    qint64 __shot__;
    qint64 __file_class__;
    qint64 __file_type__;

    episodesPtrW p_ptrw_eps;
    shotPtrW p_ptrw_shot;
    fileClassPtrW p_ptrw_fileClass;
    fileTypePtrW p_ptrw_fileType;
};

CORE_DNAMESPACE_E
