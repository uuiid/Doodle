#pragma once

#include "core_global.h"
#include "filesqlinfo.h"

CORE_NAMESPACE_S

class CORE_EXPORT assFileSqlInfo : public fileSqlInfo {
  Q_GADGET
 public:
  assFileSqlInfo();
  void select(qint64 &ID_);

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static assInfoPtrList getAll(const fileClassPtr &fc_);
  static assInfoPtrList getAll(const assTypePtr &AT_);
  static assInfoPtrList getAll(const fileTypePtr &ft_);

  QString generatePath(const QString &programFodler) override;
  QString generatePath(const QString &programFolder, const QString &suffixes) override;
  QString generatePath(const QString &programFolder, const QString &suffixes, const QString &prefix) override;
  QString generateFileName(const QString &suffixes) override;
  QString generateFileName(const QString &suffixes, const QString &prefix) override;

  fileClassPtr getFileClass();
  void setFileClass(const fileClassPtrW &fileclass_);

  fileTypePtr getFileType();
  void setFileType(const fileTypePtrW &fileType_);

  assTypePtr getAssType();
  void setAssType(const assTypePtrW &assType_);

 private:
  static assInfoPtrList batchQuerySelect(sqlQuertPtr &query);

 private:
  qint64 __file_class__;
  qint64 __file_type__;
  qint64 __ass_class__;

  fileClassPtrW p_ptrW_fileClass;
  fileTypePtrW p_ptrW_fileType;
  assTypePtrW p_ptrW_assType;

};

CORE_DNAMESPACE_E

Q_DECLARE_METATYPE(doCore::assInfoPtr)