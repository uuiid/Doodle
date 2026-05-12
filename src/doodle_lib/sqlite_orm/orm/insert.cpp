#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/insert.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <fmt/format.h>

namespace doodle::orm {

insert_t& insert_t::operator()() {
  if (!stmt_) {
    auto l_values = fmt::format("({})", fmt::join(columns_, ", "));
    if (batch_size_ > 1) l_values = fmt::format("{}", fmt::join(std::vector<std::string>(batch_size_, l_values), ", "));
    auto l_sql = fmt::format("INSERT INTO {} ({}) VALUES ({})", into_table_name_, fmt::join(columns_, ", "), l_values);
    stmt_      = std::make_shared<sqlite_stmt>();
    stmt_->prepare(*s_, l_sql);
  }
  for (size_t i = 0; i < values_.size(); ++i) {
    stmt_->bind(values_[i]);
  }
  stmt_->step();
  return *this;
}

}  // namespace doodle::orm