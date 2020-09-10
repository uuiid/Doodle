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
    QfileListPtr list_(new QVector<QFileInfo>());
    QJsonDocument jsondoc = QJsonDocument::fromBinaryData(filepathP.toUtf8());
    if(jsondoc.isNull()){
        list_->append(QFileInfo(filepathP));
    }else {
        for(QJsonValueRef x :jsondoc.array()){
            list_->append(QFileInfo(x.toString()));
        }
    }
    return list_;
}

void fileSqlInfo::setFileList(const QfileListPtr filelist)
{
    QJsonArray jsonList;
    for (QFileInfo d: *filelist.data()){
        jsonList.append(d.absolutePath());
    }
    QJsonDocument jsondoc(jsonList);
    filepathP = QString(jsondoc.toJson());
}

episodesPtr fileSqlInfo::episdes() const
{

}

CORE_DNAMESPACE_E
