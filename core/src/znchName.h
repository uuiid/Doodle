#pragma once

#include "core_global.h"

#include "coresqldata.h"

#include "pinyin_global.h"
#include "assClass.h"
CORE_NAMESPACE_S

class CORE_EXPORT znchName : public coresqldata {
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
  std::string  nameEN;
  dopinyin::convertPtr con;

  assClass *p_ptr_assType;
};

CORE_NAMESPACE_E

