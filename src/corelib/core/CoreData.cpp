﻿/*
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
  rttr::registration::class_<CoreData>(DOCORE_RTTE_CLASS(CoreData));  //DOCORE_RTTE_CLASS(CoreData)
                                                                            // .constructor<>()(rttr::policy::ctor::as_std_shared_ptr)
}

DOODLE_NAMESPACE_E
