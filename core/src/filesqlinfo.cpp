#include "filesqlinfo.h"

#include "coreset.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonValueRef>
#include <QVariant>


CORE_NAMESPACE_S



fileSqlInfo::fileSqlInfo()
{
    fileP = "";
    fileSuffixesP = "";
    userP = coreSet::getCoreSet().getUser();
    versionP = 0;
    filepathP = "";
    infoP = "";
    fileStateP = "";
}

QfileInfoVector fileSqlInfo::getFileList() const
{
    QfileInfoVector list_;
    QJsonDocument jsondoc = QJsonDocument::fromJson(filepathP.toUtf8());
    if(jsondoc.isNull()){
        list_.append(QFileInfo(filepathP));
    }else {
        for(QJsonValueRef x :jsondoc.array()){
            list_.append(QFileInfo(x.toString()));
        }
    }
    return list_;
}

void fileSqlInfo::setFileList(const QfileInfoVector& filelist)
{
    if(filelist.size() == 0){throw std::runtime_error("filelist not value");}
    QJsonArray jsonList;
    for (QFileInfo d: filelist){
        jsonList.append(d.absoluteFilePath());
    }
    QJsonDocument jsondoc(jsonList);
    filepathP = QString(jsondoc.toJson());
    fileP = filelist[0].fileName();
    fileSuffixesP = filelist[0].suffix();
}

int fileSqlInfo::getVersionP() const
{
    return versionP;
}

void fileSqlInfo::setVersionP(const int &value)
{
    versionP = value;
}

QString fileSqlInfo::getInfoP() const
{
    return infoP;
}

void fileSqlInfo::setInfoP(const QString &value)
{
    infoP = value;
}

QString fileSqlInfo::getFileStateP() const
{
    return fileStateP;
}

void fileSqlInfo::setFileStateP(const QString &value)
{
    fileStateP = value;
}

QString fileSqlInfo::formatPath(const QString &value) const
{
    return QFileInfo(value).filePath();
}



CORE_DNAMESPACE_E
