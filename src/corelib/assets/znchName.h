/*
 * @Author: your name
 * @Date: 2020-09-24 17:34:08
 * @LastEditTime: 2020-12-14 11:45:20
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\znchName.h
 */
#pragma once

#include <corelib/core_global.h>
#include <pinyinlib/pinyin_global.h>
#include <corelib/assets/assClass.h>
#include <corelib/core/CoreData.h>

DOODLE_NAMESPACE_S

class CORE_API znchName : public CoreData {
 public:
  explicit znchName(assClass *at_);

  bool setInfo(const std::string &value) override;

  void setName(const std::string &name_);
  void setName(const std::string &name_, const bool &isZNCH);
  [[nodiscard]] std::string getName() const;
  [[nodiscard]] std::string pinyin() const;

  friend assClassPtrList assClass::getAll(const assDepPtr &ass_dep_ptr);

 private:
  std::string nameZNCH;
  std::string nameEN;

  assClass *p_ptr_assType;

  RTTR_ENABLE(CoreData)
};

DOODLE_NAMESPACE_E
