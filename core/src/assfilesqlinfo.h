#pragma once

#include "core_global.h"
#include "filesqlinfo.h"

CORE_NAMESPACE_S

 class CORE_API assFileSqlInfo : public fileSqlInfo , private std::enable_shared_from_this<assFileSqlInfo>{
  Q_GADGET
 public:
  assFileSqlInfo();
  void select(qint64 &ID_);

  void insert() override;
  void updateSQL() override;

  static assInfoPtrList getAll(const assClassPtr &AT_);
  static assInfoPtrList getAll(const assTypePtr &ft_);

  dpath generatePath(const std::string &programFolder) override;
  dpath generatePath(const dstring &programFolder, const dstring &suffixes) override;
  dpath generatePath(const dstring &programFolder, const dstring &suffixes, const dstring &prefix) override;
  dstring generateFileName(const dstring &suffixes) override;
  dstring generateFileName(const dstring &suffixes, const dstring &prefix) override;

  assDepPtr getAssDep();
  void setAssDep(const assDepPtr &ass_dep_);

  const assClassPtr & getAssClass();
  void setAssClass(const assClassPtr &class_ptr);

  const assTypePtr & getAssType();
  void setAssType(const assTypePtr &ass_type_ptr);

 private:
  template<typename T>
  void batchSetAttr(const T& row);

 private:
  int64_t ass_class_id;
  int64_t ass_type_id;

  assDepPtr p_dep_ptr_;
  assClassPtr p_class_ptr_;
  assTypePtr p_type_ptr_;

};

CORE_NAMESPACE_E
