/*
 * @Author: your name
 * @Date: 2020-09-18 17:14:11
 * @LastEditTime: 2020-12-14 13:31:13
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\core\CoreData.h
 */
#pragma once

#include <corelib/core_global.h>
#include <rttr/type>

DOODLE_NAMESPACE_S

class CORE_API CoreData : public boost::noncopyable_::noncopyable {
  RTTR_ENABLE();

 public:
  CoreData();

  virtual void insert()    = 0;
  virtual void updateSQL() = 0;
  virtual void deleteSQL() = 0;
};

DOODLE_NAMESPACE_E
