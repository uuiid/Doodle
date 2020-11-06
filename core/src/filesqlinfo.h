#pragma once

#include "core_global.h"
#include "coresqldata.h"

#include <QVector>
#include <QFileInfo>
#include <QSharedPointer>
#include <QWeakPointer>

CORE_NAMESPACE_S

class CORE_EXPORT fileSqlInfo : public coresqldata {
  Q_GADGET
 public:
  //属性设置和查询
  fileSqlInfo();
  [[nodiscard]] QfileInfoVector getFileList() const;
  virtual void setFileList(const QfileInfoVector &filelist);
  virtual void setFileList(const stringList &filelist);
  [[nodiscard]] int getVersionP() const;
  void setVersionP(const int &value);

  [[nodiscard]] QJsonArray getInfoP() const;
  void setInfoP(const QString &value);

  [[nodiscard]] QString getFileStateP() const;
  void setFileStateP(const QString &value);

  [[nodiscard]] QString getSuffixes() const;

  [[nodiscard]] QString getUserP() const;

  virtual dpath generatePath(const std::string &programFodler) = 0;
  virtual dpath generatePath(const dstring &programFolder, const dstring &suffixes) = 0;
  virtual dpath generatePath(const dstring &programFolder, const dstring &suffixes, const dstring &prefix) = 0;
  virtual dstring generateFileName(const dstring &suffixes) = 0;
  virtual dstring generateFileName(const dstring &suffixes, const dstring &prefix) = 0;

 protected:
  //属性包装

  std::string  fileP;
  std::string  fileSuffixesP;
  std::string  userP;
  int versionP;
  std::string  filepathP;
  std::string  infoP;
  std::string  fileStateP;

 private:
  [[nodiscard]] QJsonDocument convertJson() const;
};

CORE_NAMESPACE_E
