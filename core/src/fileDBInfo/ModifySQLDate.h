#pragma once

#include <core_global.h>
#include <set>
DOODLE_NAMESPACE_S

class ModifySQLDate {
 public:
  ModifySQLDate();

  virtual void selectModify() = 0;
  virtual bool inDeadline(const int64_t& info_id);

 protected:
  std::set<int64_t> p_set_id;
};

DOODLE_NAMESPACE_E