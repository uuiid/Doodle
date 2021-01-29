//
// Created by teXiao on 2020/11/10.
//
#pragma once

#include "corelib/core_global.h"
#include "movieArchive.h"

DOODLE_NAMESPACE_S
class CORE_API moveShotA : public movieArchive {
 public:
  explicit moveShotA(shotInfoPtr info_ptr);

 protected:
  void insertDB() override;
  void setInfoAttr() override;
};

DOODLE_NAMESPACE_E