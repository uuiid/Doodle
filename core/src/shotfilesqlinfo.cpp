#include "shotfilesqlinfo.h"


#include "coreset.h"
#include "episodes.h"
#include "shot.h"


#include <QVariant>
#include <QDebug>
#include <QSqlError>
//#include <QxOrm_Impl.h>


CORE_NAMESPACE_S
//选择信息
const QString shotFileSqlInfo::SQLSelectCOM ="SELECT "
                                         "id,file,fileSuffixes,user,version,_file_path_,infor,filestate,__episodes__,__shot__,__file_class__,__file_type__"
                                         "FROM %1.basefile "
                                         "WHERE id=%2;";
//插入信息
//const QString shotFileSqlInfo::SQLSelectEPS = "SELECT id, episodes FROM %1.episodes WHERE id=%2";
const QString shotFileSqlInfo::SQLInsert = "INSERT INTO %1.basefile(file, fileSuffixes, user, version, _file_path_, infor, filestate, __episodes__, __shot__, __file_class__, __file_type__) "
                                           "VALUE (%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12);";
//跟新信息
const QString shotFileSqlInfo::SQLUpdata = "UPDATE %1.basefile "
                                           "SET filestate=%2,infor=%3,__episodes__=%4,__shot__=%5,__file_class__=%6,__file_type__=%7 "
                                           "WHERE id=%8;";
const QString shotFileSqlInfo::SQLCreateTable = "";

shotFileSqlInfo::shotFileSqlInfo()
{
    userP = coreSet::getCoreSet().getUser();
    __episodes__   = -1;
    __shot__       = -1;
    __file_class__ = -1;
    __file_type__  = -1;
}

shotFileSqlInfo::shotFileSqlInfo(const qint64 &ID_)
{
    QString sql = SQLSelectCOM.arg(coreSet::getCoreSet().getProjectname())
            .arg(ID_);
    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if(!query->exec(sql)) return;
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
    }
}

void shotFileSqlInfo::insert()
{
    if(idP < 0){
        QString sql = SQLInsert.arg(coreSet::getCoreSet().getProjectname());
        sqlQuertPtr query = coreSql::getCoreSql().getquery();
        query->prepare(sql);
        query->bindValue(":file",fileP);
        query->bindValue(":fileSuffixes",fileSuffixesP);
        query->bindValue(":user",userP);
        query->bindValue(":version",versionP);
        query->bindValue(":_file_path_",filepathP);
        query->bindValue(":infor",infoP);
        if(__episodes__   > 0 ){ query->bindValue(":__episodes__",__episodes__);}
        else {query->bindValue(":__episodes__",QVariant(QVariant::Int));}

        if(__shot__       > 0 ) query->bindValue(":__shot__",__shot__);
        else {query->bindValue(":__episodes__",QVariant(QVariant::Int));}

        if(__file_class__ > 0 ) query->bindValue(":__file_class__",__file_class__);
        else {query->bindValue(":__episodes__",QVariant(QVariant::Int));}

        if(__file_type__  > 0 ) query->bindValue(":__file_type__",__file_type__);
        else {query->bindValue(":__episodes__",QVariant(QVariant::Int));}

        if(!query->exec()) throw std::runtime_error("not insert fileInfo");

        qDebug() << query->lastError().text();
        query->finish();
    }else {
        updata();
    }

}

void shotFileSqlInfo::updata()
{
    QString sql = SQLInsert.arg(coreSet::getCoreSet().getProjectname())
            .arg(fileStateP)
            .arg(infoP)
            .arg(__episodes__)
            .arg(__shot__)
            .arg(__file_class__)
            .arg(__file_type__);
    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if(!query->exec(sql)) throw  std::runtime_error("not updata fileinfo");
    query->finish();
}

void shotFileSqlInfo::deleteSQL()
{

}

episodesPtr shotFileSqlInfo::getEisdes() const
{
    if(epsP != nullptr) {return epsP;}
    else {return episodesPtr(new episodes(__episodes__));}
    return nullptr;
}

void shotFileSqlInfo::setEpisdes(const episodesPtrW &eps_)
{
    epsP = eps_;
}

shotPtr shotFileSqlInfo::getShot() const
{
    if(shotP != nullptr){return shotP;}
    else{return shotPtr(new shot(__shot__));}
    return nullptr;
}

void shotFileSqlInfo::setShot(const shotPtrW &shot_)
{
    shotP = shot_;
}

CORE_DNAMESPACE_E
