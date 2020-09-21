#pragma once

#include "core_global.h"
#include "coresqldata.h"


CORE_NAMESPACE_S


class CORE_EXPORT fileType :public coresqldata
{
public:
    fileType();
    fileType(const qint64 & ID_);

    void insert() override;
    void updateSQL() override;
    void deleteSQL() override;
    
    void setType(const QString &value);
    QString getType() const;

    void setFileClass(const fileClassPtrW & fileclass_);
    fileClassPtr getFileclass();

    void setAssType(const assTypePtrW & assType_);
    assTypePtr getAssType();

    void setEpisdes(const episodesPtrW &value);
    episodesPtr getEpisdes();

    void setShot(const shotPtrW & shot_);
    shotPtr getShot();

private:
    QString p_Str_Type;
    fileClassPtrW p_fileClass;
    assTypePtrW   p_assType;
    episodesPtrW p_episdes;
    shotPtrW p_shot;

    qint64 __file_class__;
    qint64 __ass_class__;
    qint64 __episodes__;
    qint64 __shot__;
};

CORE_DNAMESPACE_E
