#include "column_operations.h"

#include <fmt/format.h>

namespace doodle::orm {
std::string operator_compare_t::to_sql(const storage& s) const {
  return fmt::format("({} {} {})", left_(s), op_, right_(s));
}
void operator_compare_t::bind(sqlite_stmt& stmt) const {
  bind_left_(stmt);
  bind_right_(stmt);
}
}  // namespace doodle::orm