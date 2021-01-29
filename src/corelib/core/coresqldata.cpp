/*
 * @Author: your name
 * @Date: 2020-09-18 17:14:11
 * @LastEditTime: 2020-12-10 16:28:42
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\core\coresqldata.cpp
 */
#include "coresqldata.h"

#include <rttr/registration>

DOODLE_NAMESPACE_S

RTTR_REGISTRATION {
  rttr::registration::class_<coresqldata>(DOCORE_RTTE_CLASS(coresqldata))  //DOCORE_RTTE_CLASS(coresqldata)
      // .constructor<>()(rttr::policy::ctor::as_std_shared_ptr)
      .property_readonly(DOODLE_TOS(getIdP), &coresqldata::getIdP)
      .method(DOODLE_TOS(isNULL), &coresqldata::isNULL)
      .method(DOODLE_TOS(isInsert), &coresqldata::isInsert);
}

coresqldata::coresqldata() {
  idP = -1;
}

qint64 coresqldata::getIdP() const {
  if (idP > 0)
    return idP;
  else
    return 0;
}

bool coresqldata::isNULL() const {
  return idP <= 0;
}

DOODLE_NAMESPACE_E
