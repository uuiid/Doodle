//
// Created by TD on 25-2-20.
//

#include "sqlite_database_impl.h"

#include <boost/scope/scope_exit.hpp>

#include <sqlite_orm/sqlite_orm.h>
namespace doodle::details {
sqlite_orm_type make_storage_doodle_impl(const std::string& in_path, sqlite_database_impl* impl) {
  return make_storage_doodle(in_path, impl);
}
void on_storage_open(sqlite3* in_ptr, sqlite_database_impl* impl) {
  impl->raw_sqlite_handle_ = in_ptr;
  default_logger_raw()->info("数据库连接已打开");
}

}  // namespace doodle::details

namespace doodle {
fts5_api* sqlite_database_impl::get_fts5_api() {
  DOODLE_CHICK(raw_sqlite_handle_ != nullptr, "数据库未打开");
  auto l_db_handle    = static_cast<sqlite3*>(raw_sqlite_handle_);
  fts5_api* pRet      = 0;
  sqlite3_stmt* pStmt = 0;
  boost::scope::scope_exit stmt_guard([&]() {
    if (pStmt) sqlite3_finalize(pStmt);
  });

  if (SQLITE_OK == sqlite3_prepare(l_db_handle, "SELECT fts5(?1)", -1, &pStmt, 0)) {
    sqlite3_bind_pointer(pStmt, 1, (void*)&pRet, "fts5_api_ptr", NULL);
    sqlite3_step(pStmt);
  } else {
    // 处理错误，例如记录日志
    auto l_msg = sqlite3_errmsg(l_db_handle);
    SPDLOG_ERROR("Failed to prepare statement: {}", l_msg);
    throw_exception(doodle_error{l_msg});
  }
  return pRet;
}

}  // namespace doodle