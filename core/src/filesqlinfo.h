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

  [[nodiscard]] int getVersionP() const;
  void setVersionP(const int &value);

  [[nodiscard]] QJsonArray getInfoP() const;
  void setInfoP(const QString &value);

  [[nodiscard]] QString getFileStateP() const;
  void setFileStateP(const QString &value);

  [[nodiscard]] QString getSuffixes() const;

  [[nodiscard]] QString getUserP() const;

  virtual QString generatePath(const QString &programFodler) = 0;
  virtual QString generatePath(const QString &programFolder, const QString &suffixes) = 0;
  virtual QString generatePath(const QString &programFolder, const QString &suffixes, const QString &prefix) = 0;
  virtual QString generateFileName(const QString &suffixes) = 0;
  virtual QString generateFileName(const QString &suffixes, const QString &prefix) = 0;

 protected:
  //属性包装
  QString fileP;
  QString fileSuffixesP;
  QString userP;
  int versionP;
  QString filepathP;
  QByteArray infoP;
  QString fileStateP;

 protected:
  [[nodiscard]] QString formatPath(const QString &value) const;
 private:
  [[nodiscard]] QJsonDocument convertJson() const;
};

CORE_NAMESPACE_E
