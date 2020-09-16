#include "assfilesqlinfo.h"
#include "coreset.h"
#include "fileclass.h"
#include "filetype.h"
#include "asstype.h"

#include <QVariant>


CORE_NAMESPACE_S



const QString assFileSqlInfo::SQLSelectCOM ="SELECT "
                                         "id,file,fileSuffixes,user,version,_file_path_,infor,filestate,__file_class__,__file_type__,__ass_class__ "
                                         "FROM %1.basefile "
                                         "WHERE id=%2";
assFileSqlInfo::assFileSqlInfo()
{
    userP = coreSet::getCoreSet().getUser();
}

assFileSqlInfo::assFileSqlInfo(qint64 &ID_)
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
        __file_class__= query->value(8).toInt();
        __file_type__ = query->value(9).toInt();
        __ass_class__ = query->value(10).toInt();
    }
}

fileClassPtr assFileSqlInfo::getFileclass() const
{
    if(fileClassP != nullptr){return fileClassP;}
    else{return fileClassPtr(new fileClass(__file_class__));}
    return nullptr;
}

void assFileSqlInfo::setFileClass(const fileClassPtrW &fileclass_)
{
    fileClassP = fileclass_;
}

fileTypePtr assFileSqlInfo::getFileType() const
{
    if(fileTypeP != nullptr){return fileTypeP;}
    else{return fileTypePtr(new fileType(__file_type__));}
    return nullptr;
}

void assFileSqlInfo::setFileType(const fileTypePtrW &fileType_)
{
    fileTypeP = fileType_;
}

assTypePtr assFileSqlInfo::getAssType() const
{
    if(assTypeP != nullptr){return assTypeP;}
    else {return assTypePtr(new assType(__ass_class__));}
    return nullptr;
}

void assFileSqlInfo::setAssType(const assTypePtrW &assType_)
{

}

CORE_DNAMESPACE_E
