#include "ModifySQLDate.h"

#include <src/Exception/Exception.h>
#include <src/fileDBInfo/filesqlinfo.h>

DOODLE_NAMESPACE_S

ModifySQLDate::ModifySQLDate()
    : p_set_id() {
}

bool ModifySQLDate::inDeadline(const int64_t& k_id_) const noexcept {
  auto it = std::find(p_set_id.begin(), p_set_id.end(), k_id_);

  return it != p_set_id.end();
}

DOODLE_NAMESPACE_E