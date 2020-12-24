//
// Created by teXiao on 2020/11/10.
//
#pragma once

#include "core_global.h"
#include "movieArchive.h"

CORE_NAMESPACE_S
class CORE_API moveShotA : public movieArchive {
 public:
  explicit moveShotA(shotInfoPtr info_ptr);

 protected:
  void insertDB() override;
  void setInfoAttr() override;
};

CORE_NAMESPACE_E