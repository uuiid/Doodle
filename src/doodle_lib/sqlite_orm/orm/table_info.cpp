#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>
#include <doodle_lib/sqlite_orm/orm/table_info.h>

namespace doodle::orm {
// std::string table_info_t::get_table_name(const storage& s) const { return s.get_table_name(type_index_); }
std::string table_info_t::to_sql(const storage& s, const to_sql_ctx& ctx) const {
  return s.get_table_name(type_index_);
}
void table_info_t::collect_bind_variants(bind_value_collector_t& bind_variants) const {}

}  // namespace doodle::orm