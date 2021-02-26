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

#include <corelib/core_global.h>
#include <corelib/core/CoreData.h>

DOODLE_NAMESPACE_S
class CORE_API assdepartment
    : public CoreData,
      public std::enable_shared_from_this<assdepartment> {
 private:
  std::string p_assDep;
  DOODLE_INSRANCE(assdepartment);

  std::vector<std::string> p_alias;

  bool merge(const assdepartment &other);

 public:
  explicit assdepartment();
  ~assdepartment();

  bool setInfo(const std::string &value) override;

  static assDepPtrList getAll();
  [[nodiscard]] const std::string &getAssDep() const;
  void setAssDep(const std::string &s_ass_dep);

  static std::unordered_set<assdepartment *> Instances();

  RTTR_ENABLE(CoreData);
};

DOODLE_NAMESPACE_E
