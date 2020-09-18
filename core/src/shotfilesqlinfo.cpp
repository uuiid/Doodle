#include "shotfilesqlinfo.h"


#include "coreset.h"
#include "episodes.h"
#include "shot.h"
#include "sql_builder/sql.h"

#include <QVariant>
#include <QDebug>
#include <QSqlError>
//#include <QxOrm_Impl.h>


CORE_NAMESPACE_S
//选择信息

const QString shotFileSqlInfo::SQLCreateTable = "";

shotFileSqlInfo::shotFileSqlInfo()
{
    __episodes__   = -1;
    __shot__       = -1;
    __file_class__ = -1;
    __file_type__  = -1;
}

shotFileSqlInfo::shotFileSqlInfo(const qint64 &ID_)
{
    sql::SelectModel select;
    select.select("id","file","fileSuffixes","user","version",
                  "_file_path_","infor","filestate",
                  "__episodes__","__shot__","__file_class__","__file_type__");

    select.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    select.where(sql::column("id") == ID_);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if(!query->exec(QString::fromStdString(select.str()))) return;
    if(query->next()){
        idP           = query->value(0).toInt();
        fileP         = query->value(1).toString();
        fileSuffixesP = query->value(2).toString();
        userP         = query->value(3).toString();
        versionP      = query->value(4).toInt();
        filepathP     = query->value(5).toString();
        infoP         = query->value(6).toString();
        fileStateP    = query->value(7).toString();
        __episodes__  = query->value(8).toInt();
        __shot__      = query->value(9).toInt();
        __file_class__= query->value(10).toInt();
        __file_type__ = query->value(11).toInt();
        return;
    }
    //失败保护
    idP = -1;
}

void shotFileSqlInfo::insert()
{
    sql::InsertModel ins_;
    if(idP < 0){
        sqlQuertPtr query = coreSql::getCoreSql().getquery();
        ins_.insert("file",fileP.toStdString());
        ins_.insert("fileSuffixes",fileSuffixesP.toStdString());
        ins_.insert("user",userP.toStdString());
        ins_.insert("version",(unsigned char)versionP);
        ins_.insert("_file_path_",filepathP.toStdString());

        if(!infoP.isEmpty())
        ins_.insert("infor",infoP.toStdString());

        if(!fileStateP.isEmpty())
        ins_.insert("filestate",fileStateP.toStdString());

        if(__episodes__   > 0 )
            ins_.insert("__episodes__",(unsigned char)__episodes__);
        if(__shot__       > 0 )
            ins_.insert("__shot__",(unsigned char)__shot__);
        if(__file_class__ > 0 )
            ins_.insert("__file_class__",(unsigned char)__file_class__);
        if(__file_type__  > 0 )
            ins_.insert("__file_type__",(unsigned char)__file_type__);

        ins_.into(coreSet::getCoreSet().getProjectname().toStdString() + ".basefile");

        qDebug() << QString::fromStdString(ins_.str());
        if(!query->exec(QString::fromStdString(ins_.str())))
            throw std::runtime_error(query->lastError().text().toStdString());

        qDebug() << query->lastError().text();
        query->finish();
    }else {
        updata();
    }

}

void shotFileSqlInfo::updata()
{
    sql::UpdateModel updatasql_;
    updatasql_.update(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    updatasql_.set("filestate",fileSuffixesP.toStdString());
    updatasql_.set("infor",infoP.toStdString());
    updatasql_.set("__episodes__",__episodes__);
    updatasql_.set("__shot__",__shot__);
    updatasql_.set("__file_class__",__file_class__);
    updatasql_.set("__file_type__",__file_type__);

    updatasql_.where(sql::column("id") == idP);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if(!query->exec(QString::fromStdString(updatasql_.str()))) throw  std::runtime_error("not updata fileinfo");
    query->finish();
}

void shotFileSqlInfo::deleteSQL()
{

}

episodesPtr shotFileSqlInfo::getEpisdes()
{
    if(eps_ptrW != nullptr) {return eps_ptrW;}
    else {
        episodesPtr p_ = episodesPtr(new episodes(__episodes__));
        this->setEpisdes(p_);
        return p_;
    }
    return nullptr;
}

void shotFileSqlInfo::setEpisdes(const episodesPtrW &eps_)
{
    eps_ptrW = eps_;
    __episodes__ = eps_.lock()->getIdP();
}

shotPtr shotFileSqlInfo::getShot()
{
    if(shot_ptrW != nullptr){return shot_ptrW;}
    else{
        shotPtrW p_ = shotPtr(new shot(__shot__));
        shot_ptrW = p_;
        return p_;
    }
    return nullptr;
}

void shotFileSqlInfo::setShot(const shotPtrW &shot_)
{
    shot_ptrW = shot_;
    __shot__ = shot_.lock()->getIdP();
}

CORE_DNAMESPACE_E
