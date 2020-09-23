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
    //使用id直接从数据库创建类
    shot(const qint64 &ID_);

    //数据库语句发出
    void insert() override;
    void updateSQL() override;
    void deleteSQL() override;

    //使用episodes 的外键约束创建多个类
    static shotPtrList getAll(const episodesPtr &EP_);

    //设置episodes约束外键
    void setEpisdes(const episodesPtrW &value);
    //获得episodes 约束实体
    episodesPtr getEpisdes();

    //设置shot自身信息
    void setShot(const qint64 &sh, const e_shotAB &ab = e_shotAB::_);
    QString getShot_str() const;
private:
    qint64 p_qint_shot_;
    e_shotAB p_qenm_shotab;

    episodesPtrW p_ptr_eps;
    qint64 __episodes__;

};

CORE_DNAMESPACE_E
