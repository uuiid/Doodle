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
    scenes,
    UE4,
    rig,
    scenes_low,
    sourceimages,
    screenshot,
    presetLight,
    None
  };

 public:
  explicit assType();
  ~assType();
  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static assTypePtrList getAll();
  //寻找想要的type类型，找不到就插入
  static assTypePtr findType(const e_type &typeName, bool autoInstall);
  static bool sortType(const assTypePtr &t1, const assTypePtr &t2);

  //这个信号用来发出更改
  static boost::signals2::signal<void()> insertChanged;

 public:
  // std::tuple<e_type, std::string> getType() const;
  [[nodiscard]] const std::string getTypeS() const;
  [[nodiscard]] const e_type &getType_enum() const;
  [[nodiscard]] void setType(const e_type &type_enum);
  [[nodiscard]] const QString getTypeQ() const;
  void setType(const std::string &string);
  static const std::unordered_set<assType *> Instances();

 private:
  static assTypePtr findType(const std::string &typeName);
  e_type s_type;

 private:
  int64_t p_ass_class_id;

  DOODLE_INSRANCE(assType);
  RTTR_ENABLE(coresqldata);
};

inline const QString assType::getTypeQ() const {
  return QString::fromStdString(getTypeS());
}
DOODLE_NAMESPACE_E