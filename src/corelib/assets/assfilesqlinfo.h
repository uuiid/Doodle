/*
 * @Author: your name
 * @Date: 2020-09-15 11:02:09
 * @LastEditTime: 2020-12-14 16:38:34
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\assfilesqlinfo.h
 */
#pragma once

#include <corelib/core_global.h>
#include <corelib/core/FileInfo.h>

//导入boost信号
#include <boost/signals2.hpp>

DOODLE_NAMESPACE_S

class CORE_API assFileSqlInfo
    : public fileSqlInfo,
      public std::enable_shared_from_this<assFileSqlInfo> {
 public:
  assFileSqlInfo();
  ~assFileSqlInfo();

  bool setInfo(const std::string &value) override;

  static assInfoPtrList getAll(const assClassPtr &AT_);

  fileSys::path generatePath(const std::string &programFolder) override;
  fileSys::path generatePath(const dstring &programFolder,
                             const dstring &suffixes) override;
  fileSys::path generatePath(const dstring &programFolder, const dstring &suffixes,
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

  virtual dataInfoPtr findSimilar() override;

  static bool sortType(const assInfoPtr &t1, const assInfoPtr &t2);
  static const std::unordered_set<assFileSqlInfo *> Instances();

  static boost::signals2::signal<void(const assInfoPtr &)> insertChanged;
  static boost::signals2::signal<void(const assInfoPtr &)> updateChanged;

 private:
  void setAssType();
  int getMaxVecsion();

 private:
  assDepPtr p_dep_ptr_;
  assClassPtr p_class_ptr_;
  assTypePtr p_type_ptr_;
  DOODLE_INSRANCE(assFileSqlInfo);
};

DOODLE_NAMESPACE_E
