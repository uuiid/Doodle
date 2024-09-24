#include "kitsu_backend_sqlite.h"

#include <boost/asio.hpp>
//
#include <doodle_core/metadata/attendance.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
namespace doodle::http {

class kitsu_backend_sqlite::kitsu_backend_sqlite_fun {
 public:
};

void kitsu_backend_sqlite::init(const sql_connection_ptr& in_conn) {}

void kitsu_backend_sqlite::connect(entt::registry& in_registry_ptr) {}
void kitsu_backend_sqlite::disconnect() {}

void kitsu_backend_sqlite::clear() {}

void kitsu_backend_sqlite::run() {}
void kitsu_backend_sqlite::begin_save() {}

void kitsu_backend_sqlite::save() {}
}  // namespace doodle::http