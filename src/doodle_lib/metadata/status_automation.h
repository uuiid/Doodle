#pragma once

#include <doodle_core/metadata/status_automation.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/awaitable.hpp>

namespace doodle::status_automation_ns {
orm::sql_modify_statement_vector_t run(
    const status_automation& in_status_automation, const std::shared_ptr<task>& in_task, const uuid& in_person_id
);

}