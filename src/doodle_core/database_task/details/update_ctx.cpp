//
// Created by TD on 2022/6/1.
//

#include "update_ctx.h"

#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/file_sys.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_file.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/thread_pool/process_message.h>

#include "sqlpp11/ppgen.h"
#include "sqlpp11/sqlite3/insert_or.h"
#include <range/v3/all.hpp>
#include <sqlpp11/ppgen.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n::details {}  // namespace doodle::database_n::details
