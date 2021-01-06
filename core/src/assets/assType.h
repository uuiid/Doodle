/*
 * @Author: your name
 * @Date: 2020-11-06 13:15:08
 * @LastEditTime: 2020-12-14 16:42:59
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\assets\assType.h
 */
//
// Created by teXiao on 2020/11/6.
//
#pragma once

#include "core_global.h"

#include <src/core/coresqldata.h>
#include <boost/signals2.hpp>

DOODLE_NAMESPACE_S

class CORE_API assType
    : public coresqldata,
      public std::enable_shared_from_this<assType> {
 public:
  enum e_type {
    UE4,
    scenes,
    rig,
    scenes_low,
    sourceimages,
    screenshot,
  };

 public:
  explicit assType();
  ~assType();
  void select(const int64_t &ID_);
  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static assTypePtrList getAll();
  static assTypePtr findType(const e_type &typeName, bool autoInstall);
  static bool sortType(const assTypePtr &t1, const assTypePtr &t2);

  //这个信号用来发出更改
  static boost::signals2::signal<void()> insertChanged;

 private:
  static assTypePtr findType(const std::string &typeName);
  std::string s_type;

 public:
  [[nodiscard]] const std::string &getType() const;
  [[nodiscard]] const QString getTypeQ() const;
  void setType(const std::string &string);
  void setType(const QString &string);
  static const std::unordered_set<assType *> Instances();

 private:
  int64_t p_ass_class_id;

  DOODLE_INSRANCE(assType);
  RTTR_ENABLE(coresqldata);
};

inline const QString assType::getTypeQ() const {
  return QString::fromStdString(getType());
}
inline void assType::setType(const QString &string) {
  setType(string.toStdString());
}
DOODLE_NAMESPACE_E