//
// Created by teXiao on 2020/10/21.
//
#pragma once

#include "core_global.h"


CORE_NAMESPACE_S
class CORE_API archiveFactory{
 public:
  static fileArchivePtr crearte(const QString & suffix);
};


CORE_NAMESPACE_E
