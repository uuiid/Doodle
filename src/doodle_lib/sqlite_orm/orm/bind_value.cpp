#include <doodle_lib/sqlite_orm/orm/bind_value.h>
#include <doodle_lib/sqlite_orm/orm/bind_value_impl.h>
#include <doodle_lib/sqlite_orm/orm/column.h>

namespace doodle::orm {

bind_value_t::bind_value_t(column_info_ptr column_info) {
  value_    = std::move(column_info);
  bind_fun_ = [](const bind_value_t& self, sqlite_stmt& stmt) {
    throw std::runtime_error(
        "Cannot bind a column_info_ptr directly. This should be handled in the context of a column operation, not as a "
        "standalone bind value."
    );
  };
  to_string_fun_ = [](const bind_value_t& self, storage& in_storage, const to_sql_ctx& ctx) {
    auto& col_info_ptr = std::any_cast<const column_info_ptr&>(self.value_);
    return fmt::format("Column({})", col_info_ptr->get_column_name(in_storage, ctx));
  };
}

void bind_value_t::bind(sqlite_stmt& stmt) const {
  if (!bind_fun_) throw std::runtime_error("No bind function available for this value");
  bind_fun_(*this, stmt);
}
std::string bind_value_t::to_string(storage& in_storage, const to_sql_ctx& ctx) const {
  if (!to_string_fun_) throw std::runtime_error("No to_string function available for this value");
  return to_string_fun_(*this, in_storage, ctx);
}
}  // namespace doodle::orm