/*
 * @Author: your name
 * @Date: 2020-09-15 14:23:34
 * @LastEditTime: 2020-12-14 16:29:02
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\assets\assClass.h
 */
#pragma once

#include "corelib/core_global.h"

#include "corelib/core/CoreData.h"

DOODLE_NAMESPACE_S

class CORE_API assClass : public CoreData,
                          public std::enable_shared_from_this<assClass> {
 public:
  assClass();
  ~assClass();
  bool setInfo(const std::string &value) override;

  static assClassPtrList getAll(const assDepPtr &ass_dep_ptr);
  [[nodiscard]] assDepPtr getAssDep() const;
  void setAssDep(const assDepPtr &value);

  std::string getAssClass() const;

  void setAssClass(const std::string &value);

  static const std::unordered_set<assClass *> Instances();

 private:
  std::string name;
  assDepPtr p_ass_dep_ptr_;

  znchNamePtr p_ptr_znch;
  DOODLE_INSRANCE(assClass);
};

DOODLE_NAMESPACE_E
