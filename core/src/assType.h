//
// Created by teXiao on 2020/11/6.
//
#pragma once

#include "core_global.h"

#include "coresqldata.h"
CORE_NAMESPACE_S

 class CORE_API assType : public coresqldata , private std::enable_shared_from_this<assType>{
 public:
  explicit assType();
  void select(const int64_t & ID_);
  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static assTypePtrList getAll(const assClassPtr &ass_class_ptr);

 private:
  std::string s_type;
 public:
  [[nodiscard]] const std::string &getType() const;
  void setType(const std::string &string);
 private:
  int64_t p_ass_class_id;

  assClassPtr p_class_ptr_;
 public:
  [[nodiscard]] const assClassPtr &getAssClassPtr() const;
  void setAssClassPtr(const assClassPtr &class_ptr);
};

CORE_NAMESPACE_E