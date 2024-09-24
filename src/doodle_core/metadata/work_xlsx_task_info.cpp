#include "work_xlsx_task_info.h"

#include <doodle_core/database_task/details/column.h>
#include <doodle_core/lib_warp/sqlite3/sqlite3.h>

#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
//
#include "doodle_core/metadata/user.h"
namespace doodle {

// to json
void to_json(nlohmann::json& j, const work_xlsx_task_info& p) {
  j["id"]                = fmt::to_string(p.id_);
  j["start_time"]        = fmt::format("{:%FT%T}", p.start_time_.get_local_time());
  j["end_time"]          = fmt::format("{:%FT%T}", p.end_time_.get_local_time());
  j["duration"]          = p.duration_.count();
  j["remark"]            = p.remark_;
  j["user_remark"]       = p.user_remark_;
  j["kitsu_task_ref_id"] = fmt::to_string(p.kitsu_task_ref_id_);
}

}  // namespace doodle