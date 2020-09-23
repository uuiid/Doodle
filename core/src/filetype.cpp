#include "filetype.h"

#include "sql_builder/sql.h"
#include "coreset.h"
#include "fileclass.h"
#include "assType.h"
#include "episodes.h"
#include "shot.h"

#include <QVariant>
#include <QSqlError>
#include <QVector>

CORE_NAMESPACE_S

fileType::fileType()
{
    p_Str_Type = "";
    p_fileClass = nullptr;
    p_assType = nullptr;
    p_episdes = nullptr;

    __file_class__ = -1;
    __ass_class__ = -1;
    __episodes__ = -1;
    __shot__ = -1;
}

fileType::fileType(const qint64 &ID_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file_type",
                "__file_class__", "__ass_class__", "__episodes__", "__shot__");

    sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("id") == ID_);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
        return;
    if (query->next())
    {
        idP = query->value(0).toInt();
        p_Str_Type = query->value(1).toString();

        if (!query->value(2).isNull())
            __file_class__ = query->value(2).toInt();
        else
            __file_class__ = -1;

        if (!query->value(3).isNull())
            __ass_class__ = query->value(3).toInt();
        else
            __ass_class__ = -1;

        if (!query->value(4).isNull())
            __episodes__ = query->value(4).toInt();
        else
            __episodes__ = -1;

        if (!query->value(5).isNull())
            __shot__ = query->value(5).toInt();
        else
            __shot__ = -1;

        return;
    }
    idP = -1;
}

void fileType::insert()
{
    sql::InsertModel ins_;
    if (idP < 0)
    {
        sqlQuertPtr query = coreSql::getCoreSql().getquery();
        ins_.insert("file_type", p_Str_Type.toStdString());

        if (__file_class__ > 0)
            ins_.insert("__file_class__", __file_class__);
        if (__ass_class__ > 0)
            ins_.insert("__ass_class__", __ass_class__);
        if (__episodes__ > 0)
            ins_.insert("__episodes__", __episodes__);
        if (__shot__ > 0)
            ins_.insert("__shot__", __shot__);

        ins_.into(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());

        if (!query->exec(QString::fromStdString(ins_.str())))
            throw std::runtime_error(query->lastError().text().toStdString());
        getInsertID(query);

        query->finish();
    }
}

void fileType::updateSQL()
{
    sql::UpdateModel upd_;
    upd_.update(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    if (__episodes__ > 0)
        upd_.set("__episodes__", __episodes__);

    if (__shot__ > 0)
        upd_.set("__shot__", __shot__);

    if (__file_class__ > 0)
        upd_.set("__file_class__", __file_class__);

    if (__ass_class__ > 0)
        upd_.set("__ass_class__", __ass_class__);

    upd_.where(sql::column("id") == idP);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(upd_.str())))
    {
        throw std::runtime_error(query->lastError().text().toStdString());
    }
    query->finish();
}

void fileType::deleteSQL()
{
}

fileTypePtrList fileType::batchQuerySelect(sqlQuertPtr &query)
{
    fileTypePtrList list_;
    while (query->next())
    {
        fileTypePtr p_ = fileTypePtr(new fileType);
        p_->idP = query->value(0).toInt();
        p_->p_Str_Type = query->value(1).toString();
        if (!query->value(2).isNull())
            p_->__file_class__ = query->value(2).toInt();
        else
            p_->__file_class__ = -1;

        if (!query->value(3).isNull())
            p_->__ass_class__ = query->value(3).toInt();
        else
            p_->__ass_class__ = -1;

        if (!query->value(4).isNull())
            p_->__episodes__ = query->value(4).toInt();
        else
            p_->__episodes__ = -1;

        if (!query->value(5).isNull())
            p_->__shot__ = query->value(5).toInt();
        else
            p_->__shot__ = -1;

        list_.append(p_);
    }
    return list_;
}

fileTypePtrList fileType::getAll(const fileClassPtr &FT_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file_type",
                "__file_class__", "__ass_class__", "__episodes__", "__shot__");

    sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__file_class__") == FT_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
    {
        throw std::runtime_error(query->lastError().text().toStdString());
    }

    fileTypePtrList listfileTypes = batchQuerySelect(query);

    for (auto &x : listfileTypes)
    {
        x->p_fileClass = FT_.toWeakRef();
    }
    return listfileTypes;
}

fileTypePtrList fileType::getAll(const assTypePtr &AT_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file_type",
                "__file_class__", "__ass_class__", "__episodes__", "__shot__");

    sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());

    sel_.where(sql::column("__file_class__") == AT_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
    {
        throw std::runtime_error(query->lastError().text().toStdString());
    }

    fileTypePtrList listfileTypes = batchQuerySelect(query);

    for (auto &x : listfileTypes)
    {
        x->p_assType = AT_.toWeakRef();
    }
    return listfileTypes;
}

fileTypePtrList fileType::getAll(const episodesPtr &EP_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file_type",
                "__file_class__", "__ass_class__", "__episodes__", "__shot__");

    sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());

    sel_.where(sql::column("__file_class__") == EP_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
    {
        throw std::runtime_error(query->lastError().text().toStdString());
    }

    fileTypePtrList listfileTypes = batchQuerySelect(query);

    for (auto &x : listfileTypes)
    {
        x->p_episdes = EP_.toWeakRef();
    }
    return listfileTypes;
}

fileTypePtrList fileType::getAll(const shotPtr &SH_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file_type",
                "__file_class__", "__ass_class__", "__episodes__", "__shot__");

    sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());

    sel_.where(sql::column("__file_class__") == SH_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
    {
        throw std::runtime_error(query->lastError().text().toStdString());
    }

    fileTypePtrList listfileTypes = batchQuerySelect(query);

    for (auto &x : listfileTypes)
    {
        x->p_shot = SH_.toWeakRef();
    }
    return listfileTypes;
}

void fileType::setType(const QString &value)
{
    p_Str_Type = value;
}

QString fileType::getType() const
{
    return p_Str_Type;
}

void fileType::setFileClass(const fileClassPtrW &value)
{
    __file_class__ = value.lock()->getIdP();
    p_fileClass = value;
}

fileClassPtr fileType::getFileclass()
{
    if (p_fileClass)
        return p_fileClass;
    else if (__file_class__ > 0)
    {
        fileClassPtr p_ = fileClassPtr(new fileClass(__file_class__));
        p_fileClass = p_;
        return p_;
    }
    else
    {
        return nullptr;
    }
}
void fileType::setAssType(const assTypePtrW &assType_)
{
    __ass_class__ = assType_.lock()->getIdP();
    p_assType = assType_;
}

assTypePtr fileType::getAssType()
{
    if (p_assType)
        return p_assType;
    else if (__ass_class__ > 0)
    {
        assTypePtr p_ = assTypePtr(new assType(__ass_class__));
        p_assType = p_;
        return p_;
    }
    else
    {
        return nullptr;
    }
}

void fileType::setEpisdes(const episodesPtrW &value)
{
    __episodes__ = value.lock()->getIdP();
    p_episdes = value;
}

episodesPtr fileType::getEpisdes()
{
    if (p_episdes)
    {
        return p_episdes;
    }
    else if (__episodes__ > 0)
    {
        episodesPtr p_ = episodesPtr(new episodes(__episodes__));
        p_episdes = p_;
        return p_;
    }
    else
    {
        return nullptr;
    }
}

void fileType::setShot(const shotPtrW &shot_)
{
    __shot__ = shot_.lock()->getIdP();
    p_shot = shot_;
}

shotPtr fileType::getShot()
{
    if (p_shot)
    {
        return p_shot;
    }
    else if (__shot__ > 0)
    {
        shotPtr p_ = shotPtr(new shot(__shot__));
        p_shot = p_;
        return p_shot;
    }
    else
    {
        return nullptr;
    }
}

CORE_DNAMESPACE_E
