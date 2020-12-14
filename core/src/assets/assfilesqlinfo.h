/*
 * @Author: your name
 * @Date: 2020-09-15 11:02:09
 * @LastEditTime: 2020-12-14 16:38:34
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\assfilesqlinfo.h
 */
#pragma once

#include <core_global.h>
#include <src/fileDBInfo/filesqlinfo.h>

CORE_NAMESPACE_S

class CORE_API assFileSqlInfo
    : public fileSqlInfo,
      public std::enable_shared_from_this<assFileSqlInfo> {
 public:
  assFileSqlInfo();
  ~assFileSqlInfo();
  void select(qint64 &ID_);

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;
  static assInfoPtrList getAll(const assClassPtr &AT_);

  dpath generatePath(const std::string &programFolder) override;
  dpath generatePath(const dstring &programFolder,
                     const dstring &suffixes) override;
  dpath generatePath(const dstring &programFolder, const dstring &suffixes,
                     const dstring &prefix) override;
  dstring generateFileName(const dstring &suffixes) override;
  dstring generateFileName(const dstring &suffixes,
                           const dstring &prefix) override;

  assDepPtr getAssDep();
  void setAssDep(const assDepPtr &ass_dep_);

  const assClassPtr &getAssClass();
  void setAssClass(const assClassPtr &class_ptr);

  const assTypePtr &getAssType();
  void setAssType(const assTypePtr &type_ptr);
  static bool sortType(const assInfoPtr &t1, const assInfoPtr &t2);

  static const std::map<int64_t, assFileSqlInfo *> &Instances();

 private:
  void setAssType();
  template <typename T>
  void batchSetAttr(const T &row);
  int getMaxVecsion();

 private:
  int64_t ass_class_id;
  int64_t ass_type_id;

  assDepPtr p_dep_ptr_;
  assClassPtr p_class_ptr_;
  assTypePtr p_type_ptr_;
  DOODLE_INSRANCE(assFileSqlInfo);
  RTTR_ENABLE(fileSqlInfo)
};

CORE_NAMESPACE_E
