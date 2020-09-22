#include "fileclass.h"

#include "sql_builder/sql.h"

#include "coreset.h"
#include "episodes.h"
#include "shot.h"

#include "magic_enum.hpp"

#include <QVariant>
#include <QSqlError>
#include <QVector>

CORE_NAMESPACE_S

fileClass::fileClass()
{
    __shot__ = -1;
    __eps__ = -1;
}

fileClass::fileClass(const qint64 &ID_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file_class", "__shot__", "__episodes__");
    sel_.from(QString("%1.fileclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("id") == ID_);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
        return;
    if (query->next())
    {
        idP = query->value("id").toInt();
        auto tmp_fc = magic_enum::enum_cast<e_fileclass>(query->value("file_class").toString().toStdString());
        if (tmp_fc.has_value())
        {
            p_fileclass = tmp_fc.value();
        }
        else
        {
            p_fileclass = e_fileclass::_;
        }
        if (!query->value("__shot__").isNull())
            __shot__ = query->value("__shot__").toInt();
        else
            __shot__ = -1;
        if (!query->value("__episodes__").isNull())
            __eps__ = query->value("__episodes__").toInt();
        else
            __eps__ = -1;
        return;
    }
    idP = -1;
}

void fileClass::insert()
{
    sql::InsertModel ins_;
    if (idP < 0)
    {
        sqlQuertPtr query = coreSql::getCoreSql().getquery();
        std::string str_fileclass(magic_enum::enum_name(p_fileclass));
        ins_.insert("file_class", str_fileclass);
        if (__shot__ > 0)
            ins_.insert("__shot__", __shot__);
        if (__eps__ > 0)
            ins_.insert("__episodes__", __eps__);

        ins_.into(QString("%1.fileclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());

        if (!query->exec(QString::fromStdString(ins_.str())))
            throw std::runtime_error(query->lastError().text().toStdString());
        getInsertID(query);

        query->finish();
    }
}

void fileClass::updateSQL()
{
    sql::UpdateModel upd_;
    upd_.update(QString("%1.fileclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    if (__shot__ >= 0)
        upd_.set("__shot__", __shot__);
    if (__eps__ >= 0)
        upd_.set("__episodes__", __eps__);

    upd_.where(sql::column("id") == idP);
    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(upd_.str())))
        throw std::runtime_error(query->lastError().text().toStdString());
    query->finish();
}

void fileClass::deleteSQL()
{
}

fileClassPtrList fileClass::getAll()
{
    fileClassPtrList list_fileClass;

    sql::SelectModel sel_;
    sel_.select("id", "file_class", "__shot__", "__episodes__");
    sel_.from(QString("%1.fileclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__shot__").is_null());
    sel_.where(sql::column("__episodes__").is_null());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
    {
        throw std::runtime_error("not exe fileClass get all ");
    }
    while (query->next())
    {
        fileClassPtr tmp_fileClass(new fileClass);
        tmp_fileClass->idP = query->value("id").toInt();
        auto tmp_fc = magic_enum::enum_cast<e_fileclass>(query->value("file_class").toString().toStdString());
        if (tmp_fc.has_value())
        {
            tmp_fileClass->p_fileclass = tmp_fc.value();
        }
        else
        {
            tmp_fileClass->p_fileclass = e_fileclass::_;
        }
        QVariant tmp_shot = query->value("__shot__");
        if (!tmp_shot.isNull())
            tmp_fileClass->__shot__ = tmp_shot.toInt();
        QVariant tmp_eps = query->value("__episodes__");
        if (!tmp_eps.isNull())
            tmp_fileClass->__eps__ = tmp_eps.toInt();
    }
    return list_fileClass;
}

QString fileClass::getFileclass_str() const
{
    std::string str(magic_enum::enum_name(p_fileclass));
    return QString::fromStdString(str);
}

fileClass::e_fileclass fileClass::getFileclass() const
{
    return p_fileclass;
}

void fileClass::setFileclass(const e_fileclass &value)
{
    p_fileclass = value;
}

void fileClass::setFileclass(const QString &value)
{
    auto tmp_fc = magic_enum::enum_cast<e_fileclass>(value.toStdString());
    if (tmp_fc.has_value())
    {
        p_fileclass = tmp_fc.value();
    }
    else
    {
        throw std::runtime_error("not file class in enum");
    }
}

episodesPtr fileClass::getEpisodes()
{
    if (eps_ptrW != nullptr)
    {
        return eps_ptrW;
    }
    else if (__eps__ > 0)
    {
        episodesPtr p_ = episodesPtr(new episodes(__eps__));
        eps_ptrW = p_;
        return p_;
    }
    else
    {
        return nullptr;
    }
}

void fileClass::setEpisodes(const episodesPtrW &value)
{
    eps_ptrW = value;
    __eps__ = value.lock()->getIdP();
}

shotPtr fileClass::getShot()
{
    if (shot_ptrW != nullptr && shot_ptrW.isNull())
    {
        return shot_ptrW;
    }
    else if (__shot__ > 0)
    {
        shotPtr p_ = shotPtr(new shot(__shot__));
        shot_ptrW = p_;
        return p_;
    }
    else
    {
        return nullptr;
    }
}

void fileClass::setShot(const shotPtrW &value)
{
    shot_ptrW = value;
    __shot__ = value.lock()->getIdP();
}

CORE_DNAMESPACE_E
