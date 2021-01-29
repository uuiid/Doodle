/*
 * @Author: your name
 * @Date: 2020-09-18 17:14:11
 * @LastEditTime: 2020-12-14 13:31:13
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\core\coresqldata.h
 */
#pragma once

#include <corelib/core_global.h>
#include <rttr/type>

DOODLE_NAMESPACE_S

class CORE_API coresqldata : public boost::noncopyable_::noncopyable {
 public:
  coresqldata();

  virtual void insert()    = 0;
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

DOODLE_NAMESPACE_E
