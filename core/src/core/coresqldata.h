/*
 * @Author: your name
 * @Date: 2020-09-18 17:14:11
 * @LastEditTime: 2020-12-08 19:44:14
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\core\coresqldata.h
 */
#pragma once

#include "core_global.h"
#include <rttr/type>

CORE_NAMESPACE_S

class CORE_API coresqldata {
 public:
  coresqldata();

  virtual void insert() = 0;
  virtual void updateSQL() = 0;
  virtual void deleteSQL() = 0;

  [[nodiscard]] int64_t getIdP() const;

  [[nodiscard]] bool isNULL() const;
  [[nodiscard]] inline bool isInsert() const;

 protected:
  qint64 idP;

  RTTR_ENABLE();
};

bool coresqldata::isInsert() const { return !isNULL(); }

CORE_NAMESPACE_E
