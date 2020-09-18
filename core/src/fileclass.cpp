#include "fileclass.h"

#include "sql_builder/sql.h"
#include "coreset.h"

#include "episodes.h"
#include "shot.h"

#include <QVariant>
#include <QSqlError>

CORE_NAMESPACE_S

fileClass::fileClass()
{
    __shot__ = -1;
    __eps__ = -1;
}

fileClass::fileClass(const qint64 &ID_)
{
    sql::SelectModel sel_;
    sel_.select("id","file_class","__shot__","__episodes__");
    sel_.from(QString("%1.fileclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("id") == ID_);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if(!query->exec(QString::fromStdString(sel_.str()))) return;
    if(query->next()){
        idP = query->value("id").toInt();
        filecalssP = query->value("file_class").toString();
        __shot__ = query->value("__shot__").toInt();
        __eps__  = query->value("__episodes__").toInt();
        return;
    }
    idP = -1;
}

void fileClass::insert()
{
    sql::InsertModel ins_;
    if(idP < 0){
        sqlQuertPtr query = coreSql::getCoreSql().getquery();
        ins_.insert("file_class",filecalssP.toStdString());
        if(__shot__ > 0)
            ins_.insert("__shot__",__shot__);
        if(__eps__  > 0)
            ins_.insert("__episodes__",__eps__);

        ins_.into(coreSet::getCoreSet().getProjectname().toStdString() + ".fileclass");

        if(!query->exec(QString::fromStdString(ins_.str())))
            throw std::runtime_error(query->lastError().text().toStdString());

        query->finish();
    }else {
        updata();
    }
}

void fileClass::updata()
{
    sql::UpdateModel upd_;
    upd_.update(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());

    upd_.set("")
}

void fileClass::deleteSQL()
{

}

QString fileClass::getFilecalssP() const
{
    return filecalssP;
}

void fileClass::setFilecalssP(const QString &value)
{
    filecalssP = value;
}

episodesPtr fileClass::getEps_ptrW()
{
    if(eps_ptrW != nullptr) {return eps_ptrW;}
    else {
        episodesPtr p_ = episodesPtr(new episodes(__eps__));
        eps_ptrW = p_;
        return p_;
    }
}

void fileClass::setEps_ptrW(const episodesPtrW &value)
{
    eps_ptrW = value;
    __eps__ = value.lock()->getIdP();
}

shotPtr fileClass::getShot_ptrW()
{
    if(shot_ptrW != nullptr && shot_ptrW.isNull()) {return shot_ptrW;}
    else {
        shotPtr p_ = shotPtr(new shot(__shot__));
        shot_ptrW = p_;
        return p_;
    }
}

void fileClass::setShot_ptrW(const shotPtrW &value)
{
    shot_ptrW = value;
    __shot__ = value.lock()->getIdP();
}

qint64 fileClass::getIdP()
{
    return idP;
}

CORE_DNAMESPACE_E
