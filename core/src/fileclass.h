#pragma once

#include "core_global.h"

#include "coresqldata.h"


CORE_NAMESPACE_S


class CORE_EXPORT fileClass :public coresqldata
{
public:
    fileClass();
    fileClass(const qint64 & ID_);

    enum class e_fileclass {
        _ = 0,
        Executive = 1,
        Light = 2,
        VFX  = 3,
        modle = 4,
        rig = 5,
        Anm = 6,
        direct =7,
        paint = 8,
    };

    void insert() override;
    void updateSQL() override;
    void deleteSQL() override;

    static fileClassPtrList getAll();

    QString getFileclass_str() const;
    e_fileclass getFileclass() const;
    void setFileclass(const e_fileclass &value);
    void setFileclass(const QString &value);

    episodesPtr getEpisodes();
    void setEpisodes(const episodesPtrW &value);

    shotPtr getShot();
    void setShot(const shotPtrW &value);

private:
    e_fileclass p_fileclass;

    episodesPtrW eps_ptrW;
    shotPtrW shot_ptrW;
    
    qint64 __shot__;
    qint64 __eps__;
};

CORE_DNAMESPACE_E
