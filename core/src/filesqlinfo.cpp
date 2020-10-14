#include "filesqlinfo.h"

#include "coreset.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonValueRef>
#include <QVariant>

CORE_NAMESPACE_S

fileSqlInfo::fileSqlInfo() {
  fileP = "";
  fileSuffixesP = "";
  userP = coreSet::getCoreSet().getUser();
  versionP = 0;
  filepathP = "";
  infoP = "";
  fileStateP = "";
}

QfileInfoVector fileSqlInfo::getFileList() const {
  QfileInfoVector list_;
  QJsonDocument jsondoc = QJsonDocument::fromJson(filepathP.toUtf8());
  if (jsondoc.isNull()) {
    list_.append(QFileInfo(filepathP));
  } else {
    for (QJsonValueRef x :jsondoc.array()) {
      list_.append(QFileInfo(x.toString()));
    }
  }
  return list_;
}

void fileSqlInfo::setFileList(const QfileInfoVector &filelist) {
  if (filelist.size() == 0) { throw std::runtime_error("filelist not value"); }
  QJsonArray jsonList;
  for (QFileInfo d: filelist) {
    jsonList.append(d.absoluteFilePath());
  }
  QJsonDocument jsondoc(jsonList);
  filepathP = QString(jsondoc.toJson(QJsonDocument::Compact));
  fileP = filelist[0].fileName();
  fileSuffixesP = filelist[0].suffix();
}

int fileSqlInfo::getVersionP() const {
  return versionP;
}

void fileSqlInfo::setVersionP(const int &value) {
  versionP = value;
}

QJsonArray fileSqlInfo::getInfoP() const {
  QJsonDocument json_document = convertJson();
  if (json_document.isArray())
    return json_document.array();
  else
    return QJsonArray();
}

void fileSqlInfo::setInfoP(const QString &value) {
  auto json_document = convertJson();
  if(json_document.isArray()){
    auto arr = json_document.array();
    arr.push_back(value);
    json_document.setArray(arr);
  }
  infoP = json_document.toJson(QJsonDocument::Compact);
}

QString fileSqlInfo::getFileStateP() const {
  return fileStateP;
}

void fileSqlInfo::setFileStateP(const QString &value) {
  fileStateP = value;
}

QString fileSqlInfo::formatPath(const QString &value) const {
  return QFileInfo(value).filePath();
}
QString fileSqlInfo::getUserP() const {
  return userP;
}
QJsonDocument fileSqlInfo::convertJson() const {
  QJsonDocument json_document = QJsonDocument::fromJson(infoP);
  if (!json_document.isNull()) {
    return json_document;
  } else{
    auto json_array = QJsonArray();
    json_array.push_back(QJsonValue(QString(infoP)));
    json_document.setArray(json_array);
    return json_document;
  }
}
QString fileSqlInfo::getSuffixes() const {
  return fileSuffixesP;
}

CORE_DNAMESPACE_E
