#include "sql_builder/sql.h"

#include "asstype.h"
#include "coreSet.h"
#include "fileclass.h"

#include <QVariant>
#include <QSqlError>

CORE_NAMESPACE_S

assType::assType()
{
    name ="";
    __file_class__ = -1;
    filassP = nullptr;
}

assType::assType(const qint64 &ID_)
{
    sql::SelectModel sel_;
    sel_.select("id","file_name","__file_class__");
    sel_.from(QString("%1.assclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("id") == ID_);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();

    if(!query->exec(QString::fromStdString(sel_.str()))) return;
    if(query->next()){
        idP            = query->value(0).toInt();
        name      = query->value(1).toString();
        __file_class__ = query->value(2).toInt();
        return;
    }
    //添加失败保护
    __file_class__ = -1;
}

void assType::insert()
{
    sql::InsertModel ins_;
    if(idP < 0){
        sqlQuertPtr query = coreSql::getCoreSql().getquery();
        ins_.insert("file_name", name.toStdString());
        if(__file_class__ >= 0)
            ins_.insert("__file_class__", __file_class__);

        ins_.into(QString("%1.assclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
        
        if(!query->exec(QString::fromStdString(ins_.str()))) 
            throw std::runtime_error(query->lastError().text().toStdString());
        query->finish();

    }else{
        updateSQL();
    }
}

void assType::updateSQL()
{
    sql::UpdateModel upd_;

    upd_.update(QString("%1.assclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());

    upd_.set("__file_class__", __file_class__);

    upd_.where(sql::column("id") == idP);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if(!query->exec(QString::fromStdString(upd_.str())))
        throw std::runtime_error(query->lastError().text().toStdString());
    query->finish();
}

void assType::deleteSQL()
{
    
}

fileClassPtr assType::getFile_class()
{
     if(filassP != nullptr) {return filassP;}
     else {
        fileClassPtr p_ = fileClassPtr(new fileClass(__file_class__));
        this->filassP = p_;
        return p_;
     }
}

void assType::setFile_class(const fileClassPtrW &value)
{
    filassP = value;
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
