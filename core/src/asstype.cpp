#include "sql_builder/sql.h"

#include "asstype.h"
#include "coreSet.h"
#include "fileclass.h"

#include <QVariant>
#include <QSqlError>
#include <QVector>

CORE_NAMESPACE_S

assType::assType()
{
    name = "";
    __file_class__ = -1;
    p_tprw_fileClass = nullptr;
}

assType::assType(const qint64 &ID_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file_name", "__file_class__");
    sel_.from(QString("%1.assclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("id") == ID_);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();

    if (!query->exec(QString::fromStdString(sel_.str())))
        return;
    if (query->next())
    {
        idP = query->value(0).toInt();
        name = query->value(1).toString();
        __file_class__ = query->value(2).toInt();
        return;
    }
    //添加失败保护
    __file_class__ = -1;
}

void assType::insert()
{
    sql::InsertModel ins_;
    if (idP < 0)
    {
        sqlQuertPtr query = coreSql::getCoreSql().getquery();
        ins_.insert("file_name", name.toStdString());
        if (__file_class__ >= 0)
            ins_.insert("__file_class__", __file_class__);

        ins_.into(QString("%1.assclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());

        if (!query->exec(QString::fromStdString(ins_.str())))
            throw std::runtime_error(query->lastError().text().toStdString());
        getInsertID(query);
        query->finish();
    }
}

void assType::updateSQL()
{
    sql::UpdateModel upd_;

    upd_.update(QString("%1.assclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());

    upd_.set("__file_class__", __file_class__);

    upd_.where(sql::column("id") == idP);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(upd_.str())))
        throw std::runtime_error(query->lastError().text().toStdString());
    query->finish();
}

void assType::deleteSQL()
{
}

assTypePtrList assType::batchQuerySelect(sqlQuertPtr & query)
{
    assTypePtrList listassType;
    while (query->next())
    {
        assTypePtr p_;
        p_->idP = query->value(0).toInt();
        p_->name = query->value(1).toString();
        p_->__file_class__ = query->value(2).toInt();
        listassType.append(p_);
    }
    return listassType;
}

assTypePtrList assType::getAll(const fileClassPtr &fc_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file_name", "__file_class__");
    sel_.from(QString("%1.assclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__file_class__") == fc_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();

    if (!query->exec(QString::fromStdString(sel_.str())))
        return;
    
    assTypePtrList listassType = batchQuerySelect(query);
    for (auto &x : listassType)
    {
        x->p_tprw_fileClass = fc_.toWeakRef();
    }
    return listassType;
}

fileClassPtr assType::getFile_class()
{
    if (p_tprw_fileClass != nullptr)
    {
        return p_tprw_fileClass;
    }
    else
    {
        fileClassPtr p_ = fileClassPtr(new fileClass(__file_class__));
        this->p_tprw_fileClass = p_;
        return p_;
    }
}

void assType::setFile_class(const fileClassPtrW &value)
{
    p_tprw_fileClass = value;
    __file_class__ = value.lock()->getIdP();
}

void assType::setName(const QString &value)
{
    name = value;
}

QString assType::getName() const
{
    return name;
}

CORE_DNAMESPACE_E
