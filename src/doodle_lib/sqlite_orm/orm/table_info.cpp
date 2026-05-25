#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>
#include <doodle_lib/sqlite_orm/orm/table_info.h>


namespace doodle::orm {
std::string table_info_t ::get_table_name(const storage& s) const { return s.get_table_name(type_index_); }

}  // namespace doodle::orm