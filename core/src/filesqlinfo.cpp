#include "filesqlinfo.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonValueRef>

CORE_NAMESPACE_S

fileSqlInfo::fileSqlInfo()
{
}

QfileListPtr fileSqlInfo::fileListGet() const
{
    QfileListPtr list_;
    QJsonDocument jsondoc = QJsonDocument::fromBinaryData(filepathP.toUtf8());
    if(jsondoc.isNull()){
        list_.append(QFileInfo(filepathP));
    }else {
        for(QJsonValueRef x :jsondoc.array()){
            list_.append(QFileInfo(x.toString()));
        }
    }
    return list_;
}

void fileSqlInfo::setFileList(const QfileListPtr filelist)
{
    QJsonArray jsonList;
    for (QFileInfo d: filelist){
        jsonList.append(d.absolutePath());
    }
    QJsonDocument jsondoc(jsonList);
    filepathP = QString(jsondoc.toJson());
}

episodesPtr fileSqlInfo::episdes() const
{
    return nullptr;
}

void fileSqlInfo::setEpisdes(const episodesPtr &eps_)
{

}

shotPtr fileSqlInfo::shot() const
{
return nullptr;
}

void fileSqlInfo::setshot(const shotPtr &shot_)
{

}

fileClassPtr fileSqlInfo::fileclass() const
{
return nullptr;
}

void fileSqlInfo::setFileClass(const fileClassPtr &fileclass_)
{

}

fileTypePtr fileSqlInfo::fileType() const
{
return nullptr;
}

void fileSqlInfo::setFileType(const fileTypePtr &fileType_)
{

}

assTypePtr fileSqlInfo::assType() const
{
return nullptr;
}

void fileSqlInfo::setAssType(const assTypePtr &assType_)
{

}

CORE_DNAMESPACE_E
