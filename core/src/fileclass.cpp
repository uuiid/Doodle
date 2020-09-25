#include "fileclass.h"

#include "sql_builder/sql.h"

#include "coreset.h"
#include "episodes.h"
#include "shot.h"

#include "magic_enum.hpp"

#include <QVariant>
#include <QSqlError>
#include <QVector>

#include <iostream>

CORE_NAMESPACE_S

fileClass::fileClass()
{
    p_fileclass = e_fileclass::_;

    __shot__ = -1;
    __eps__ = -1;

    p_ptrW_eps = nullptr;
    p_ptrW_shot = nullptr;
}

void fileClass::select(const qint64 &ID_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file_class", "__shot__", "__episodes__");
    sel_.from(QString("%1.fileclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("id") == ID_);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
        throw std::runtime_error(query->lastError().text().toStdString());
    if (query->next())
    {
        idP = query->value("id").toInt();
        auto tmp_fc = magic_enum::enum_cast<e_fileclass>(query->value("file_class").toString().toStdString());
        if (tmp_fc.has_value())
        {
            p_fileclass = tmp_fc.value();
        }

        if (!query->value("__shot__").isNull())
            __shot__ = query->value("__shot__").toInt();

        if (!query->value("__episodes__").isNull())
            __eps__ = query->value("__episodes__").toInt();
    }
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

fileClassPtrList fileClass::batchQuerySelect(sqlQuertPtr &query)
{
    fileClassPtrList list_fileClass;
    while (query->next())
    {
        fileClassPtr tmp_fileClass(new fileClass);
        tmp_fileClass->idP = query->value("id").toInt();
        auto tmp_fc = magic_enum::enum_cast<e_fileclass>(query->value("file_class").toString().toStdString());
        if (tmp_fc.has_value())
        {
            tmp_fileClass->p_fileclass = tmp_fc.value();
        }

        QVariant tmp_shot = query->value("__shot__");
        if (!tmp_shot.isNull())
            tmp_fileClass->__shot__ = tmp_shot.toInt();
            
        QVariant tmp_eps = query->value("__episodes__");
        if (!tmp_eps.isNull())
            tmp_fileClass->__eps__ = tmp_eps.toInt();

        list_fileClass.append(tmp_fileClass);
    }
    return list_fileClass;
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

    return batchQuerySelect(query);
}

fileClassPtrList fileClass::getAll(const episodesPtr &EP_)
{

    sql::SelectModel sel_;
    sel_.select("id", "file_class", "__shot__", "__episodes__");
    sel_.from(QString("%1.fileclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__episodes__") == EP_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
    {
        throw std::runtime_error("not exe fileClass get all ");
    }

    fileClassPtrList listFileClass = batchQuerySelect(query);
    for (auto &x : listFileClass)
    {
        x->p_ptrW_eps = EP_.toWeakRef();
    }
    return listFileClass;
}

fileClassPtrList fileClass::getAll(const shotPtr &SH_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file_class", "__shot__", "__episodes__");
    sel_.from(QString("%1.fileclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__shot__") == SH_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
    {
        throw std::runtime_error("not exe fileClass get all ");
    }

    fileClassPtrList listFileClass = batchQuerySelect(query);
    for (auto &x : listFileClass)
    {
        x->p_ptrW_shot = SH_.toWeakRef();
    }
    return listFileClass;
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
    if (p_ptrW_eps != nullptr)
    {
        return p_ptrW_eps;
    }
    else if (__eps__ > 0)
    {
        episodesPtr p_ = episodesPtr(new episodes);
        p_->select(__eps__);
        p_ptrW_eps = p_;
        return p_;
    }
    else
    {
        return nullptr;
    }
}

void fileClass::setEpisodes(const episodesPtrW &value)
{
    try
    {
        p_ptrW_eps = value;
        __eps__ = value.lock()->getIdP();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

shotPtr fileClass::getShot()
{
    if (p_ptrW_shot != nullptr && p_ptrW_shot.isNull())
    {
        return p_ptrW_shot;
    }
    else if (__shot__ > 0)
    {
        shotPtr p_ = shotPtr(new shot);
        p_->select(__shot__);
        p_ptrW_shot = p_;
        return p_;
    }
    else
    {
        return nullptr;
    }
}

void fileClass::setShot(const shotPtrW &value)
{
    try
    {
        p_ptrW_shot = value;
        __shot__ = value.lock()->getIdP();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    setEpisodes(value.lock()->getEpisodes());
}

CORE_DNAMESPACE_E
