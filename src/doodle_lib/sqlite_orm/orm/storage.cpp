#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <string>
#include <vector>

namespace doodle {
namespace orm {

storage& storage::finalize() {
  for (std::size_t i = 0; i < tables_.size(); ++i) {
    auto& table                              = tables_[i];
    type_to_table_index_[table->type_index_] = i;
  }
  return *this;
}

}  // namespace orm

}  // namespace doodle