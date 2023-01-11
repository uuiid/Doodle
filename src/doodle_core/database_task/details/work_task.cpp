#include "work_task.h"

#include <doodle_core/generate/core/sql_sql.h>

#include <boost/preprocessor.hpp>
#include <boost/preprocessor/facilities/expand.hpp>

#include <sqlpp11/ppgen.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <stdint.h>
// namespace sql {

// // clang-format off
// SQLPP_DECLARE_TABLE(
//     (work_task_info),
//     (id  , std::int32_t, SQLPP_PRIMARY_KEY)
//     (time, datetime    , SQLPP_NULL   )

// )
// // clang-format on
// }  // namespace sql

namespace doodle::database_n {

void sql_com<doodle::work_task_info, false>::create_table(conn_ptr& in_ptr) {
  static std::once_flag l_f{};
  std::call_once(l_f, [&]() {
    auto l_f = cmrc::DoodleLibResource::get_filesystem().open("core/sql_file.sql");

    in_ptr->execute(std::string{l_f.cbegin(), l_f.cend()});
  });
}
void sql_com<doodle::work_task_info, false>::install(conn_ptr& in_ptr, const entt::handle& in_handle) {}
void sql_com<doodle::work_task_info, false>::update(conn_ptr& in_ptr, const entt::handle& in_handle) {}
void sql_com<doodle::work_task_info, false>::select(conn_ptr& in_ptr, entt::handle& in_handle) {}
void sql_com<doodle::work_task_info, false>::destroy(conn_ptr& in_ptr, const entt::handle& in_handle) {}

}  // namespace doodle::database_n