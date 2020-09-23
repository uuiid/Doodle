#include "shot.h"

#include "sql_builder/sql.h"
#include "episodes.h"
#include "coreset.h"

#include <magic_enum.hpp>

#include <QVariant>
#include <QSqlError>
#include <QVector>

CORE_NAMESPACE_S

shot::shot()
{
    p_qint_shot_ = -1;
    p_qenm_shotab = e_shotAB::_;

    __episodes__ = -1;
}

shot::shot(const qint64 &ID_)
{
    sql::SelectModel sel_;
    sel_.select("id", "shot_", "shotab", "__episodes__");
    sel_.from(QString("%1.shot").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("id") == ID_);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();

    if (!query->exec(QString::fromStdString(sel_.str())))
        return;
    if (query->next())
    {
        idP = query->value(0).toInt();
        p_qint_shot_ = query->value(1).toInt();
        auto ab = magic_enum::enum_cast<e_shotAB>(query->value(2).toString().toStdString());
        if (ab.has_value())
        {
            p_qenm_shotab = ab.value();
        }
        else
        {
            p_qenm_shotab = e_shotAB::_;
        }
        if (!query->value(2).isNull())
            __episodes__ = query->value(2).toInt();
        else
            __episodes__ = -1;
        return;
    }
    idP = -1;
}

void shot::setEpisdes(const episodesPtrW &value)
{
    __episodes__ = value.lock()->getIdP();
    p_ptr_eps = value;
}

episodesPtr shot::getEpisdes()
{
    if (p_ptr_eps)
        return p_ptr_eps.lock();
    else
    {
        episodesPtr p_ = episodesPtr(new episodes(__episodes__));
        p_ptr_eps = p_.toWeakRef();
        return p_;
    }
}

void shot::setShot(const qint64 &sh, const e_shotAB &ab)
{
    p_qint_shot_ = sh;
    p_qenm_shotab = ab;
}

void shot::insert()
{
    sql::InsertModel ins_;
    if (idP < 0)
    {
        sqlQuertPtr query = coreSql::getCoreSql().getquery();
        ins_.insert("shot_", p_qint_shot_);
        std::string str(magic_enum::enum_name(p_qenm_shotab));
        ins_.insert("shotab", str);
        if (__episodes__ > 0)
            ins_.insert("__episodes__", __episodes__);

        ins_.into(QString("%1.shot").arg(coreSet::getCoreSet().getProjectname()).toStdString());
        if (!query->exec(QString::fromStdString(ins_.str())))
            throw std::runtime_error(query->lastError().text().toStdString());
        getInsertID(query);
        query->finish();
    }
}

void shot::updateSQL()
{
    sql::UpdateModel upd_;
    upd_.update(QString("%1.shot").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    if (__episodes__ >= 0)
        upd_.set("__episodes__", __episodes__);

    upd_.where(sql::column("id") == idP);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(upd_.str())))
        throw std::runtime_error(query->lastError().text().toStdString());
    query->finish();
}

void shot::deleteSQL()
{
}

shotPtrList shot::getAll(const episodesPtr &EP_)
{
    //创建选择sql语句
    sql::SelectModel sel_;
    sel_.select("id", "shot_", "shotab", "__episodes__");
    sel_.from(QString("%1.shot").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__episodes__") == EP_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    //如果获得就抛出异常
    if (!query->exec(QString::fromStdString(sel_.str())))
        throw std::runtime_error(query->lastError().text().toStdString());
    
    shotPtrList listShot;
    while (query->next())
    {
        shotPtr sh_(new shot);
        sh_->idP = query->value(0).toInt();
        sh_->p_qint_shot_ = query->value(1).toInt();
        //转换并检查枚举值
        auto ab = magic_enum::enum_cast<e_shotAB>(query->value(2).toString().toStdString());
        if (ab.has_value())
            sh_->p_qenm_shotab = ab.value();
        else
            sh_->p_qenm_shotab = e_shotAB::_;

        //连接外键和实体约束
        sh_->__episodes__ = query->value(3).toInt();
        sh_->p_ptr_eps = EP_.toWeakRef();
        listShot.append(sh_);
    }

    return listShot;
}
CORE_DNAMESPACE_E
