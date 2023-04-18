//
// Created by TD on 2022/5/30.
//

#include "insert.h"

#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/database_task/details/update_ctx.h>
#include <doodle_core/database_task/sql_file.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/detail/time_point_info.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_task.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <database_task/details/com_data.h>
#include <range/v3/all.hpp>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {}  // namespace doodle::database_n
