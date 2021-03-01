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
#include <corelib/core/coreset.h>
#include <rttr/type>

DOODLE_NAMESPACE_S

namespace pathParser {
class PathParser;
}

class CORE_API CoreData {
  RTTR_ENABLE()

 protected:
  fileSys::path p_roots;

 public:
  CoreData();
  virtual bool setInfo(const std::string &value) = 0;

  DOODLE_DISABLE_COPY(CoreData);

  const fileSys::path &Root() const noexcept;
  virtual void setRoot(const fileSys::path &Roots) noexcept;
};

DOODLE_NAMESPACE_E
