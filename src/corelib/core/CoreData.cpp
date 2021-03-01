/*
 * @Author: your name
 * @Date: 2020-09-18 17:14:11
 * @LastEditTime: 2020-12-10 16:28:42
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\core\CoreData.cpp
 */
#include "CoreData.h"

#include <rttr/registration>

DOODLE_NAMESPACE_S

RTTR_REGISTRATION {
  rttr::registration::class_<CoreData>(DOCORE_RTTE_CLASS(CoreData))  //DOCORE_RTTE_CLASS(CoreData)
      // .method(DOODLE_TOS(setInfo), &CoreData::setInfo)               // .constructor<>()(rttr::policy::ctor::as_std_shared_ptr)
      // .property(DOODLE_TOS(p_roots), &CoreData::Roots, &CoreData::setRoots)
      ;
}
CoreData::CoreData()
    : p_roots() {
}

const fileSys::path& CoreData::Root() const noexcept {
  return p_roots;
}

void CoreData::setRoot(const fileSys::path& Roots) noexcept {
  p_roots = Roots;
}

DOODLE_NAMESPACE_E
