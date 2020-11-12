//
// Created by teXiao on 2020/11/6.
//

#pragma once

#include "core_global.h"
#include "coresqldata.h"

CORE_NAMESPACE_S
class CORE_API assdepartment : public coresqldata, private std::enable_shared_from_this<assdepartment> {
 public:
  explicit assdepartment();
  void select(const int64_t &ID_);
  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static assDepPtrList getAll();
  [[nodiscard]] const std::string &getAssDep() const;
  void setAssDep(const std::string &s_ass_dep);
 private:
  int64_t i_prjID;

  std::string s_assDep;

};

CORE_NAMESPACE_E

