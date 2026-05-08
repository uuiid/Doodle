#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <string>
#include <vector>

namespace doodle {
namespace orm {

storage& storage::finalize() {
  for (auto& table : tables_) {
    for (auto& func : table->to_register_) {
      func(*this);
    }
  }

  return *this;
}

}  // namespace orm

}  // namespace doodle