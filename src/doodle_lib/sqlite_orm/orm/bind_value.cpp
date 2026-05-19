#include <doodle_lib/sqlite_orm/orm/bind_value.h>
#include <doodle_lib/sqlite_orm/orm/bind_value_impl.h>

namespace doodle::orm {
void bind_value_t::bind(sqlite_stmt& stmt) const {
  if (!bind_fun_) throw std::runtime_error("No bind function available for this value");
  bind_fun_(*this, stmt);
}
}  // namespace doodle::orm