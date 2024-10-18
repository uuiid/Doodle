//
// Created by TD on 24-9-12.
//

#include "sqlite_database.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/sqlite_orm/detail/assets_type_enum.h>
#include <doodle_core/sqlite_orm/detail/attendance_enum.h>
#include <doodle_core/sqlite_orm/detail/macro.h>
#include <doodle_core/sqlite_orm/detail/std_chrono_duration.h>
#include <doodle_core/sqlite_orm/detail/std_chrono_time_point.h>
#include <doodle_core/sqlite_orm/detail/std_chrono_zoned_time.h>
#include <doodle_core/sqlite_orm/detail/std_filesystem_path_orm.h>
#include <doodle_core/sqlite_orm/detail/uuid_to_blob.h>

#include "metadata/attendance.h"
#include <sqlite_orm/sqlite_orm.h>
namespace doodle {

namespace {
auto make_storage_doodle(const std::string& in_path) {
  using namespace sqlite_orm;

  return std::move(make_storage(
      in_path,  //
      make_index("assets_file_tab_uuid_id_index", &assets_file_helper::database_t::uuid_id_),
      make_table(
          "assets_file_tab",  //
          make_column("id", &assets_file_helper::database_t::id_, primary_key()),
          make_column("uuid_id", &assets_file_helper::database_t::uuid_id_, unique()),
          make_column("label", &assets_file_helper::database_t::label_),
          make_column("parent_uuid", &assets_file_helper::database_t::uuid_parent_),
          make_column("path", &assets_file_helper::database_t::path_),
          make_column("notes", &assets_file_helper::database_t::notes_, null()),
          make_column("active", &assets_file_helper::database_t::active_),
          make_column("parent_id", &assets_file_helper::database_t::parent_id_),
          foreign_key(&assets_file_helper::database_t::parent_id_).references(&assets_helper::database_t::id_)
      ),

      make_index("assets_tab_uuid_id_index", &assets_helper::database_t::uuid_id_),
      make_table(
          "assets_tab",  //
          make_column("id", &assets_helper::database_t::id_, primary_key()),
          make_column("uuid_id", &assets_helper::database_t::uuid_id_, unique()),
          make_column("label", &assets_helper::database_t::label_),
          make_column("parent_uuid", &assets_helper::database_t::uuid_parent_, null())
      ),
      make_index("kitsu_task_type_tab_uuid_id_index", &metadata::kitsu::task_type_t::uuid_id_),
      make_index("kitsu_task_type_tab_kitsu_uuid_index", &metadata::kitsu::task_type_t::kitsu_uuid_),
      make_table(
          "kitsu_task_type_tab",                                                 //
          make_column("id", &metadata::kitsu::task_type_t::id_, primary_key()),  //
          make_column("uuid_id", &metadata::kitsu::task_type_t::uuid_id_, unique()),
          make_column("kitsu_uuid", &metadata::kitsu::task_type_t::kitsu_uuid_),
          make_column("name", &metadata::kitsu::task_type_t::name_),
          make_column("use_chick_files", &metadata::kitsu::task_type_t::use_chick_files)
      ),
      make_index("attendance_tab_uuid_id_index", &attendance_helper::database_t::uuid_id_),
      make_index("attendance_tab_create_date_index", &attendance_helper::database_t::create_date_),
      make_table(
          "attendance_tab",                                                       //
          make_column("id", &attendance_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &attendance_helper::database_t::uuid_id_, unique()),
          make_column("start_time", &attendance_helper::database_t::start_time_),
          make_column("end_time", &attendance_helper::database_t::end_time_),
          make_column("remark", &attendance_helper::database_t::remark_),
          make_column("att_enum", &attendance_helper::database_t::type_),
          make_column("create_date", &attendance_helper::database_t::create_date_),
          make_column("update_time", &attendance_helper::database_t::update_time_),
          make_column("dingding_id", &attendance_helper::database_t::dingding_id_),
          make_column("user_id", &attendance_helper::database_t::user_ref),
          foreign_key(&attendance_helper::database_t::user_ref).references(&user_helper::database_t::id_)
      ),

      make_index("work_xlsx_task_info_tab_year_month_index", &work_xlsx_task_info_helper::database_t::year_month_),
      make_index("work_xlsx_task_info_tab_user_index", &work_xlsx_task_info_helper::database_t::user_ref_),
      make_table(
          "work_xlsx_task_info_tab",                                                       //
          make_column("id", &work_xlsx_task_info_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &work_xlsx_task_info_helper::database_t::uuid_id_, unique()),
          make_column("start_time", &work_xlsx_task_info_helper::database_t::start_time_),
          make_column("end_time", &work_xlsx_task_info_helper::database_t::end_time_),
          make_column("duration", &work_xlsx_task_info_helper::database_t::duration_),
          make_column("remark", &work_xlsx_task_info_helper::database_t::remark_),
          make_column("user_remark", &work_xlsx_task_info_helper::database_t::user_remark_),
          make_column("year_month", &work_xlsx_task_info_helper::database_t::year_month_),
          make_column("user_id", &work_xlsx_task_info_helper::database_t::user_ref_),
          make_column("kitsu_task_ref_id", &work_xlsx_task_info_helper::database_t::kitsu_task_ref_id_),
          foreign_key(&work_xlsx_task_info_helper::database_t::user_ref_).references(&user_helper::database_t::id_)
      ),
      make_index("user_tab_dingding_index", &user_helper::database_t::dingding_id_),
      make_table(
          "user_tab",                                                       //
          make_column("id", &user_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &user_helper::database_t::uuid_id_, unique()),
          make_column("mobile", &user_helper::database_t::mobile_),  //
          make_column("dingding_id", &user_helper::database_t::dingding_id_),
          make_column("dingding_company_id", &user_helper::database_t::dingding_company_id_)
      ),

      make_index("scan_data_ue_uuid", &scan_data_t::database_t::ue_uuid_),
      make_index("scan_data_rig_uuid", &scan_data_t::database_t::rig_uuid_),
      make_index("scan_data_solve_uuid", &scan_data_t::database_t::solve_uuid_),
      make_table(
          "scan_data",  //
          make_column("id", &scan_data_t::database_t::id_, primary_key()),
          make_column("uuid_id", &scan_data_t::database_t::uuid_id_, unique()),
          make_column("ue_uuid", &scan_data_t::database_t::ue_uuid_),
          make_column("rig_uuid", &scan_data_t::database_t::rig_uuid_),
          make_column("solve_uuid", &scan_data_t::database_t::solve_uuid_),

          make_column("ue_path", &scan_data_t::database_t::ue_path_),
          make_column("rig_path", &scan_data_t::database_t::rig_path_),
          make_column("solve_path", &scan_data_t::database_t::solve_path_),

          make_column("season", &scan_data_t::database_t::season_),
          make_column("assets_type_enum", &scan_data_t::database_t::dep_),

          make_column("project", &scan_data_t::database_t::project_id_),
          make_column("name", &scan_data_t::database_t::name_),
          make_column("version", &scan_data_t::database_t::version_),
          make_column("num", &scan_data_t::database_t::num_),  //
          // make_column("file_hash", &scan_data_t::database_t::hash_),
          foreign_key(&scan_data_t::database_t::project_id_).references(&project_helper::database_t::id_)
      ),  //
      make_index("project_tab_uuid", &project_helper::database_t::uuid_id_),
      make_index("project_tab_kitsu_uuid", &project_helper::database_t::kitsu_uuid_),
      make_table(
          "project_tab",                                                       //
          make_column("id", &project_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &project_helper::database_t::uuid_id_, unique()),
          make_column("name", &project_helper::database_t::name_),  //
          make_column("path", &project_helper::database_t::path_),
          make_column("en_str", &project_helper::database_t::en_str_),  //
          make_column("shor_str", &project_helper::database_t::shor_str_),
          make_column("local_path", &project_helper::database_t::local_path_),
          make_column("auto_upload_path", &project_helper::database_t::auto_upload_path_),
          make_column("kitsu_uuid", &project_helper::database_t::kitsu_uuid_)
      )
  ));
}
using sqlite_orm_type = decltype(make_storage_doodle(""));

auto get_cast_storage(const std::shared_ptr<void>& in_storage) {
  return std::static_pointer_cast<sqlite_orm_type>(in_storage);
}
constexpr std::size_t g_step_size{500};
}  // namespace

#define DOODLE_TO_SQLITE_THREAD()                                 \
  auto this_executor = co_await boost::asio::this_coro::executor; \
  co_await boost::asio::post(boost::asio::bind_executor(*strand_, boost::asio::use_awaitable));

void sqlite_database::set_path(const FSys::path& in_path) {
  strand_        = std::make_shared<strand_type>(boost::asio::make_strand(g_io_context()));
  storage_any_   = std::make_shared<sqlite_orm_type>(std::move(make_storage_doodle(in_path.generic_string())));
  auto l_storage = get_cast_storage(storage_any_);
  l_storage->open_forever();
  l_storage->sync_schema(true);
  default_logger_raw()->info("sql thread safe {} ", sqlite_orm::threadsafe());
}

std::vector<scan_data_t::database_t> sqlite_database::find_by_path_id(const uuid& in_id) {
  using namespace sqlite_orm;
  auto l_storage = get_cast_storage(storage_any_);
  return l_storage->get_all<scan_data_t::database_t>(sqlite_orm::where(
      sqlite_orm::c(&scan_data_t::database_t::ue_uuid_) == in_id ||
      sqlite_orm::c(&scan_data_t::database_t::rig_uuid_) == in_id || c(&scan_data_t::database_t::solve_uuid_) == in_id
  ));
}

std::vector<project_helper::database_t> sqlite_database::find_project_by_name(const std::string& in_name) {
  using namespace sqlite_orm;
  auto l_storage = get_cast_storage(storage_any_);
  return l_storage->get_all<project_helper::database_t>(
      sqlite_orm::where(sqlite_orm::c(&project_helper::database_t::name_) == in_name)
  );
}

std::vector<attendance_helper::database_t> sqlite_database::get_attendance(
    const std::int64_t& in_ref_id, const chrono::local_days& in_data
) {
  using namespace sqlite_orm;
  auto l_storage = get_cast_storage(storage_any_);

  return l_storage->get_all<attendance_helper::database_t>(where(
      c(&attendance_helper::database_t::user_ref) == in_ref_id &&
      c(&attendance_helper::database_t::create_date_) == in_data
  ));
}
std::vector<attendance_helper::database_t> sqlite_database::get_attendance(
    const std::int64_t& in_ref_id, const std::vector<chrono::local_days>& in_data
) {
  using namespace sqlite_orm;
  auto l_storage = get_cast_storage(storage_any_);
  return l_storage->get_all<attendance_helper::database_t>(where(
      c(&attendance_helper::database_t::user_ref) == in_ref_id &&
      in(&attendance_helper::database_t::create_date_, in_data)
  ));
}
std::vector<work_xlsx_task_info_helper::database_t> sqlite_database::get_work_xlsx_task_info(
    const std::int64_t& in_ref_id, const chrono::local_days& in_data
) {
  using namespace sqlite_orm;
  auto l_storage = get_cast_storage(storage_any_);
  return l_storage->get_all<work_xlsx_task_info_helper::database_t>(where(
      c(&work_xlsx_task_info_helper::database_t::user_ref_) == in_ref_id &&
      c(&work_xlsx_task_info_helper::database_t::year_month_) == in_data
  ));
}

DOODLE_GET_BY_PARENT_ID_SQL(assets_file_helper::database_t);
DOODLE_GET_BY_PARENT_ID_SQL(assets_helper::database_t);

DOODLE_UUID_TO_ID(scan_data_t::database_t)
DOODLE_UUID_TO_ID(project_helper::database_t)
DOODLE_UUID_TO_ID(user_helper::database_t)
DOODLE_UUID_TO_ID(metadata::kitsu::task_type_t)
DOODLE_UUID_TO_ID(assets_file_helper::database_t)
DOODLE_UUID_TO_ID(assets_helper::database_t)

DOODLE_GET_BY_KITSU_UUID_SQL(project_helper::database_t)
DOODLE_GET_BY_KITSU_UUID_SQL(metadata::kitsu::task_type_t)

DOODLE_GET_BY_UUID_SQL(scan_data_t::database_t)
DOODLE_GET_BY_UUID_SQL(user_helper::database_t)
DOODLE_GET_BY_UUID_SQL(work_xlsx_task_info_helper::database_t)
DOODLE_GET_BY_UUID_SQL(assets_file_helper::database_t)
DOODLE_GET_BY_UUID_SQL(assets_helper::database_t)

DOODLE_GET_ALL_SQL(project_helper::database_t)
DOODLE_GET_ALL_SQL(scan_data_t::database_t)
DOODLE_GET_ALL_SQL(user_helper::database_t)
DOODLE_GET_ALL_SQL(metadata::kitsu::task_type_t)
DOODLE_GET_ALL_SQL(assets_file_helper::database_t)
DOODLE_GET_ALL_SQL(assets_helper::database_t)

DOODLE_INSTALL_SQL(scan_data_t::database_t)
DOODLE_INSTALL_SQL(project_helper::database_t)
DOODLE_INSTALL_SQL(user_helper::database_t)
DOODLE_INSTALL_SQL(metadata::kitsu::task_type_t)
DOODLE_INSTALL_SQL(assets_file_helper::database_t)
DOODLE_INSTALL_SQL(assets_helper::database_t)

DOODLE_INSTALL_RANGE(project_helper::database_t)
DOODLE_INSTALL_RANGE(attendance_helper::database_t)
DOODLE_INSTALL_RANGE(scan_data_t::database_t)
DOODLE_INSTALL_RANGE(work_xlsx_task_info_helper::database_t)
DOODLE_INSTALL_RANGE(metadata::kitsu::task_type_t)

DOODLE_REMOVE_RANGE(scan_data_t::database_t)
DOODLE_REMOVE_RANGE(attendance_helper::database_t)
DOODLE_REMOVE_RANGE(work_xlsx_task_info_helper::database_t)
DOODLE_REMOVE_RANGE(metadata::kitsu::task_type_t)
DOODLE_REMOVE_RANGE(assets_file_helper::database_t)
DOODLE_REMOVE_RANGE(assets_helper::database_t)

DOODLE_REMOVE_BY_UUID(assets_helper::database_t)
DOODLE_REMOVE_BY_UUID(assets_file_helper::database_t)

void sqlite_database::load(const FSys::path& in_path) { set_path(in_path); }
}  // namespace doodle