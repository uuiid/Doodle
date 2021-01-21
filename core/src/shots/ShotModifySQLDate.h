#pragma once

#include <core_global.h>

#include <src/fileDBInfo/ModifySQLDate.h>
DOODLE_NAMESPACE_S

class ShotModifySQLDate : public ModifySQLDate {
 public:
  ShotModifySQLDate(std::weak_ptr<episodes> &eps);

  virtual void selectModify() override;

 private:
  std::weak_ptr<episodes> p_eps;
};

DOODLE_NAMESPACE_E