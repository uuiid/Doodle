/*
 * @Author: your name
 * @Date: 2020-09-24 17:34:08
 * @LastEditTime: 2020-11-26 17:57:50
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\znchName.h
 */
#pragma once

#include <core_global.h>
#include <pinyin_global.h>
#include <src/assets/assClass.h>
#include <src/core/coresqldata.h>

CORE_NAMESPACE_S

class CORE_API znchName : public coresqldata {
 public:
  explicit znchName(assClass *at_);

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;
  void select();

  void setName(const std::string &name_);
  void setName(const std::string &name_, const bool &isZNCH);
  [[nodiscard]] std::string getName() const;
  [[nodiscard]] std::string pinyin() const;

  friend assClassPtrList assClass::getAll(const assDepPtr &ass_dep_ptr);

 private:
  std::string nameZNCH;
  std::string nameEN;
  dopinyin::convertPtr con;

  assClass *p_ptr_assType;
};

CORE_NAMESPACE_E
