//
// Created by TD on 25-2-20.
//

#include "sqlite_database_impl.h"

#include <sqlite_orm/sqlite_orm.h>
#include <type_traits>
namespace doodle::details {
sqlite_orm_type make_storage_doodle_impl(const std::string& in_path, sqlite_database_impl* impl) {
  return make_storage_doodle(in_path, impl);
}
void on_storage_open(sqlite3* in_ptr, sqlite_database_impl* impl) {
  impl->raw_sqlite_handle_ = in_ptr;
  default_logger_raw()->info("数据库连接已打开");
}

}  // namespace doodle::details