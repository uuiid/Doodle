/*
 * @Author: your name
 * @Date: 2020-11-06 09:22:09
 * @LastEditTime: 2020-12-14 16:31:22
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\assets\assdepartment.h
 */
//
// Created by teXiao on 2020/11/6.
//

#pragma once

#include <core_global.h>
#include <src/core/coresqldata.h>

CORE_NAMESPACE_S
class CORE_API assdepartment
    : public coresqldata,
      public std::enable_shared_from_this<assdepartment> {
 public:
  explicit assdepartment();
  ~assdepartment();

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static assDepPtrList getAll();
  [[nodiscard]] const std::string &getAssDep() const;
  [[nodiscard]] const QString getAssDepQ() const;
  void setAssDep(const std::string &s_ass_dep);

  static std::map<int64_t, assdepartment *> &Instances();

  RTTR_ENABLE(coresqldata);

 private:
  int64_t i_prjID;

  std::string s_assDep;
  DOODLE_INSRANCE(assdepartment);
};

CORE_NAMESPACE_E
