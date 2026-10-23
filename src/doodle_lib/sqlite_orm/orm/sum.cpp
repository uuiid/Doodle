#include <doodle_lib/sqlite_orm/orm/session.h>
#include <doodle_lib/sqlite_orm/orm/storage_impl.h>
#include <doodle_lib/sqlite_orm/orm/sum.h>

namespace doodle::orm {
namespace {
template <typename Value>
void set_sum_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) {
  if (!out_value.has_value()) throw std::runtime_error("Output value pointer is null");
  if (out_value.type() != typeid(Value*)) throw std::runtime_error("Output value type mismatch");
  auto l_any = out_value;
  Value l_result{};
  if (!stmt.column_is_null(columnIndex)) l_result = stmt.get_column_value<Value>(columnIndex);
  *std::any_cast<Value*>(l_any) = std::move(l_result);
}
}  // namespace

std::string sum_column_info_t::get_column_name(const session& s, const to_sql_ctx& ctx) const {
  if (!column_info_) return "SUM(*)";

  return fmt::format("SUM({})", column_info_->get_column_name(s, ctx));
}

void sum_set_value<std::optional<std::int64_t>>::apply(
    const sqlite_stmt& stmt, int columnIndex, const std::any& out_value
) {
  set_sum_value<std::optional<std::int64_t>>(stmt, columnIndex, out_value);
}

void sum_set_value<std::optional<std::double_t>>::apply(
    const sqlite_stmt& stmt, int columnIndex, const std::any& out_value
) {
  set_sum_value<std::optional<std::double_t>>(stmt, columnIndex, out_value);
}

void sum_column_info_t::set_struct_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) const {
  if (!out_value.has_value()) throw std::runtime_error("Output value pointer is null");
  if (out_value.type() == typeid(std::optional<std::int64_t>*)) {
    sum_set_value<std::optional<std::int64_t>>::apply(stmt, columnIndex, out_value);
    return;
  }
  if (out_value.type() == typeid(std::optional<std::double_t>*)) {
    sum_set_value<std::optional<std::double_t>>::apply(stmt, columnIndex, out_value);
    return;
  }
  throw std::runtime_error(
      fmt::format(
          "Output value type mismatch, expected {}, got {}", typeid(std::optional<std::int64_t>*).name(),
          out_value.type().name()
      )
  );
}
}  // namespace doodle::orm
