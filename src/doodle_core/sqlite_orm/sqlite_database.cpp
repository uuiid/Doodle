//
// Created by TD on 24-9-12.
//

#include "sqlite_database.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/department.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/kitsu/assets_type.h>
#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/metadata_descriptor.h>
#include <doodle_core/metadata/preview_background_file.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/sqlite_orm/detail/macro.h>
#include <doodle_core/sqlite_orm/detail/nlohmann_json.h>
#include <doodle_core/sqlite_orm/detail/std_chrono_duration.h>
#include <doodle_core/sqlite_orm/detail/std_chrono_time_point.h>
#include <doodle_core/sqlite_orm/detail/std_chrono_zoned_time.h>
#include <doodle_core/sqlite_orm/detail/std_filesystem_path_orm.h>
#include <doodle_core/sqlite_orm/detail/uuid_to_blob.h>

#include "metadata/attendance.h"
#include "metadata/status_automation.h"
#include <sqlite_orm/sqlite_orm.h>
namespace sqlite_orm {
DOODLE_SQLITE_ENUM_TYPE_(::doodle::power_enum)
DOODLE_SQLITE_ENUM_TYPE_(::doodle::computer_status)
DOODLE_SQLITE_ENUM_TYPE_(::doodle::server_task_info_status)
DOODLE_SQLITE_ENUM_TYPE_(::doodle::server_task_info_type)
// DOODLE_SQLITE_ENUM_TYPE_(doodle::details::assets_type_enum)
DOODLE_SQLITE_ENUM_TYPE_(::doodle::details::assets_type_enum);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::attendance_helper::att_enum);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::metadata_descriptor_data_type);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::two_factor_authentication_types);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::person_role_type);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::status_automation_change_type);

template <>
struct type_is_nullable<std::string> : std::true_type {
  bool operator()(const std::string& t) const { return t.empty(); }
};
template <>
struct type_is_nullable<boost::uuids::uuid> : std::true_type {
  bool operator()(const boost::uuids::uuid& t) const { return t.is_nil(); }
};
}  // namespace sqlite_orm

#ifndef NDEBUG
#define KITSU_TABLE
#endif

namespace doodle {

namespace {
auto make_storage_doodle(const std::string& in_path) {
  using namespace sqlite_orm;

  return std::move(make_storage(
      in_path,  //
      make_index("server_task_info_tab_uuid_id_idx", &server_task_info::uuid_id_),
      make_table(
          "server_task_info_tab",  //
          make_column("id", &server_task_info::id_, primary_key()),
          make_column("uuid_id", &server_task_info::uuid_id_, unique(), not_null()),  //
          make_column("exe", &server_task_info::exe_),                                //
          make_column(
              "command", static_cast<void (server_task_info::*)(const std::string&)>(&server_task_info::sql_command),
              static_cast<const std::string& (server_task_info::*)() const>(&server_task_info::sql_command)
          ),                                                                    //
          make_column("status", &server_task_info::status_),                    //
          make_column("name", &server_task_info::name_),                        //
          make_column("source_computer", &server_task_info::source_computer_),  //
          make_column("submitter", &server_task_info::submitter_),              //
          make_column("submit_time", &server_task_info::submit_time_),          //
          make_column("run_time", &server_task_info::run_time_),                //
          make_column("end_time", &server_task_info::end_time_),                //
          make_column("run_computer_id", &server_task_info::run_computer_id_),  //
          make_column("kitsu_task_id", &server_task_info::kitsu_task_id_),      //
          make_column("type", &server_task_info::type_)
      ),
      make_index("computer_tab_uuid_id_index", &computer::uuid_id_),
      make_table(
          "computer_tab",                                                     //
          make_column("id", &computer::id_, primary_key()),                   //
          make_column("uuid_id", &computer::uuid_id_, unique(), not_null()),  //
          make_column("name", &computer::name_),                              //
          make_column("ip", &computer::ip_),                                  //
          make_column("server_status", &computer::server_status_),            //
          make_column("client_status", &computer::client_status_)
      ),
      make_index("kitsu_assets_type_tab_uuid_id_index", &metadata::kitsu::assets_type_t::uuid_id_),
      make_table(
          "kitsu_assets_type_tab",                                                 //
          make_column("id", &metadata::kitsu::assets_type_t::id_, primary_key()),  //
          make_column("uuid_id", &metadata::kitsu::assets_type_t::uuid_id_, unique(), not_null()),
          make_column("name", &metadata::kitsu::assets_type_t::name_),
          make_column("asset_type", &metadata::kitsu::assets_type_t::type_)
      ),
      make_index("assets_file_tab_uuid_id_index", &assets_file_helper::database_t::uuid_id_),
      make_table(
          "assets_file_tab",  //
          make_column("id", &assets_file_helper::database_t::id_, primary_key()),
          make_column("uuid_id", &assets_file_helper::database_t::uuid_id_, unique(), not_null()),
          make_column("label", &assets_file_helper::database_t::label_),
          make_column("parent_uuid", &assets_file_helper::database_t::uuid_parent_),
          make_column("path", &assets_file_helper::database_t::path_),
          make_column("notes", &assets_file_helper::database_t::notes_),
          make_column("active", &assets_file_helper::database_t::active_),
          make_column("parent_id", &assets_file_helper::database_t::parent_id_),
          make_column("has_thumbnail", &assets_file_helper::database_t::has_thumbnail_, default_value(false)),
          make_column("extension", &assets_file_helper::database_t::extension_, default_value(".png"s)),
          foreign_key(&assets_file_helper::database_t::parent_id_).references(&assets_helper::database_t::id_)
      ),

      make_index("assets_tab_uuid_id_index", &assets_helper::database_t::uuid_id_),
      make_table(
          "assets_tab",  //
          make_column("id", &assets_helper::database_t::id_, primary_key()),
          make_column("uuid_id", &assets_helper::database_t::uuid_id_, unique(), not_null()),
          make_column("label", &assets_helper::database_t::label_, not_null()),
          make_column("parent_uuid", &assets_helper::database_t::uuid_parent_),
          make_column("order", &assets_helper::database_t::order_, default_value(0), not_null())
      ),
      make_index("kitsu_task_type_tab_uuid_id_index", &metadata::kitsu::task_type_t::uuid_id_),
      make_table(
          "kitsu_task_type_tab",                                                 //
          make_column("id", &metadata::kitsu::task_type_t::id_, primary_key()),  //
          make_column("uuid_id", &metadata::kitsu::task_type_t::uuid_id_, unique(), not_null()),
          make_column("name", &metadata::kitsu::task_type_t::name_),
          make_column("use_chick_files", &metadata::kitsu::task_type_t::use_chick_files)
      ),
      make_index("attendance_tab_uuid_id_index", &attendance_helper::database_t::uuid_id_),
      make_index("attendance_tab_create_date_index", &attendance_helper::database_t::create_date_),
      make_table(
          "attendance_tab",                                                       //
          make_column("id", &attendance_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &attendance_helper::database_t::uuid_id_, unique(), not_null()),
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
          make_column("uuid_id", &work_xlsx_task_info_helper::database_t::uuid_id_, unique(), not_null()),
          make_column("start_time", &work_xlsx_task_info_helper::database_t::start_time_),
          make_column("end_time", &work_xlsx_task_info_helper::database_t::end_time_),
          make_column("duration", &work_xlsx_task_info_helper::database_t::duration_),
          make_column("remark", &work_xlsx_task_info_helper::database_t::remark_),
          make_column("user_remark", &work_xlsx_task_info_helper::database_t::user_remark_),
          make_column("year_month", &work_xlsx_task_info_helper::database_t::year_month_),
          make_column("user_id", &work_xlsx_task_info_helper::database_t::user_ref_),
          make_column("kitsu_task_ref_id", &work_xlsx_task_info_helper::database_t::kitsu_task_ref_id_),
          make_column("season", &work_xlsx_task_info_helper::database_t::season_),
          make_column("episode", &work_xlsx_task_info_helper::database_t::episode_),
          make_column("name", &work_xlsx_task_info_helper::database_t::name_),
          make_column("grade", &work_xlsx_task_info_helper::database_t::grade_),
          make_column("project_id", &work_xlsx_task_info_helper::database_t::project_id_),
          foreign_key(&work_xlsx_task_info_helper::database_t::user_ref_).references(&user_helper::database_t::id_)
      ),
      make_index("user_tab_dingding_index", &user_helper::database_t::dingding_id_),
      make_table(
          "user_tab",                                                       //
          make_column("id", &user_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &user_helper::database_t::uuid_id_, unique(), not_null()),
          make_column("mobile", &user_helper::database_t::mobile_),  //
          make_column("dingding_id", &user_helper::database_t::dingding_id_),
          make_column("dingding_company_id", &user_helper::database_t::dingding_company_id_),
          make_column("power", &user_helper::database_t::power_)
      ),

      make_index("project_tab_uuid", &project_helper::database_t::uuid_id_),
      make_table(
          "project_tab",                                                       //
          make_column("id", &project_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &project_helper::database_t::uuid_id_, unique(), not_null()),
          make_column("name", &project_helper::database_t::name_),  //
          make_column("path", &project_helper::database_t::path_),
          make_column("en_str", &project_helper::database_t::en_str_),                      //
          make_column("auto_upload_path", &project_helper::database_t::auto_upload_path_),  //
          make_column("code", &project_helper::database_t::code_)
      )
      /// 这个下方是模拟kitsu的表
      ,
      make_table(
          "task_type_asset_type_link",                                                           //
          make_column("id", &task_type_asset_type_link::id_, primary_key().autoincrement()),     //
          make_column("asset_type_id", &task_type_asset_type_link::asset_type_id_, not_null()),  //
          make_column("task_type_id", &task_type_asset_type_link::task_type_id_, not_null()),    //
          foreign_key(&task_type_asset_type_link::asset_type_id_).references(&asset_type::uuid_id_).on_delete.cascade(),
          foreign_key(&task_type_asset_type_link::task_type_id_).references(&task_type::uuid_id_).on_delete.cascade()
      ),
      make_table(
          "project_person_link",  //
          make_column("id", &project_person_link::id_, primary_key().autoincrement()),
          make_column("project_id", &project_person_link::project_id_, not_null()),
          make_column("person_id", &project_person_link::person_id_, not_null()),
          foreign_key(&project_person_link::project_id_).references(&project::uuid_id_),
          foreign_key(&project_person_link::person_id_).references(&person::uuid_id_)
      ),
      make_table(
          "project_task_type_link",  //
          make_column("id", &project_task_type_link::id_, primary_key().autoincrement()),
          make_column("uuid", &project_task_type_link::uuid_id_, unique(), not_null()),
          make_column("project_id", &project_task_type_link::project_id_, not_null()),
          make_column("task_type_id", &project_task_type_link::task_type_id_, not_null()),
          make_column("priority", &project_task_type_link::priority_),
          foreign_key(&project_task_type_link::project_id_).references(&project::uuid_id_).on_delete.cascade(),
          foreign_key(&project_task_type_link::task_type_id_).references(&task_type::uuid_id_).on_delete.cascade()
      ),
      make_table(
          "project_task_status_link",  //
          make_column("id", &project_task_status_link::id_, primary_key().autoincrement()),
          make_column("uuid", &project_task_status_link::uuid_id_, unique(), not_null()),
          make_column("project_id", &project_task_status_link::project_id_, not_null()),
          make_column("task_status_id", &project_task_status_link::task_status_id_, not_null()),
          make_column("priority", &project_task_status_link::priority_),
          foreign_key(&project_task_status_link::project_id_).references(&project::uuid_id_).on_delete.cascade(),
          foreign_key(&project_task_status_link::task_status_id_).references(&task_status::uuid_id_).on_delete.cascade()
      ),
      make_table(
          "project_asset_type_link",  //
          make_column("id", &project_asset_type_link::id_, primary_key().autoincrement()),
          make_column("project_id", &project_asset_type_link::project_id_, not_null()),
          make_column("asset_type_id", &project_asset_type_link::asset_type_id_, not_null()),
          foreign_key(&project_asset_type_link::project_id_).references(&project::uuid_id_).on_delete.cascade(),
          foreign_key(&project_asset_type_link::asset_type_id_).references(&asset_type::uuid_id_).on_delete.cascade()
      ),
      make_table(
          "project_status_automation_link",
          make_column("id", &project_status_automation_link::id_, primary_key().autoincrement()),
          make_column("project_id", &project_status_automation_link::project_id_, not_null()),
          make_column("status_automation_id", &project_status_automation_link::status_automation_id_, not_null()),
          foreign_key(&project_status_automation_link::project_id_).references(&project::uuid_id_).on_delete.cascade(),
          foreign_key(&project_status_automation_link::status_automation_id_)
              .references(&status_automation::uuid_id_)
              .on_delete.cascade()
      ),
      make_table(
          "project",                                                                                         //
          make_column("id", &project::id_, primary_key().autoincrement()),                                   //
          make_column("uuid", &project::uuid_id_, not_null(), unique()),                                     //
          make_column("name", &project::name_),                                                              //
          make_column("code", &project::code_),                                                              //
          make_column("description", &project::description_),                                                //
          make_column("shotgun_id", &project::shotgun_id_),                                                  //
          make_column("file_tree", &project::file_tree_),                                                    //
          make_column("data", &project::data_),                                                              //
          make_column("has_avatar", &project::has_avatar_),                                                  //
          make_column("fps", &project::fps_),                                                                //
          make_column("ratio", &project::ratio_),                                                            //
          make_column("resolution", &project::resolution_),                                                  //
          make_column("production_type", &project::production_type_),                                        //
          make_column("production_style", &project::production_style_),                                      //
          make_column("start_date", &project::start_date_),                                                  //
          make_column("end_date", &project::end_date_),                                                      //
          make_column("nb_episodes", &project::nb_episodes_),                                                //
          make_column("episode_span", &project::episode_span_),                                              //
          make_column("max_retakes", &project::max_retakes_),                                                //
          make_column("is_clients_isolated", &project::is_clients_isolated_),                                //
          make_column("is_preview_download_allowed", &project::is_preview_download_allowed_),                //
          make_column("is_set_preview_automated", &project::is_set_preview_automated_),                      //
          make_column("is_publish_default_for_artists", &project::is_publish_default_for_artists_),          //
          make_column("hd_bitrate_compression", &project::hd_bitrate_compression_),                          //
          make_column("ld_bitrate_compression", &project::ld_bitrate_compression_),                          //
          make_column("project_status_id", &project::project_status_id_),                                    //
          make_column("default_preview_background_file_id", &project::default_preview_background_file_id_),  //
          foreign_key(&project::project_status_id_).references(&project_status::uuid_id_).on_delete.cascade(),
          foreign_key(&project::default_preview_background_file_id_)
              .references(&preview_background_file::uuid_id_)
              .on_delete.cascade()
      ),
      make_table(
          "metadata_descriptor_department_link",  //
          make_column("id", &metadata_descriptor_department_link::id_, primary_key().autoincrement()),
          make_column("metadata_descriptor_id", &metadata_descriptor_department_link::metadata_descriptor_uuid_),
          make_column("department_id", &metadata_descriptor_department_link::department_uuid_),
          foreign_key(&metadata_descriptor_department_link::metadata_descriptor_uuid_)
              .references(&metadata_descriptor::uuid_id_)
              .on_delete.cascade(),
          foreign_key(&metadata_descriptor_department_link::department_uuid_)
              .references(&department::uuid_id_)
              .on_delete.cascade()
      ),
      make_table(
          "metadata_descriptor",                                                        //
          make_column("id", &metadata_descriptor::id_, primary_key().autoincrement()),  //
          make_column("uuid", &metadata_descriptor::uuid_id_, not_null(), unique()),    //
          make_column("name", &metadata_descriptor::name_),                             //
          make_column("entity_type", &metadata_descriptor::entity_type_),               //
          make_column("project_uuid", &metadata_descriptor::project_uuid_),             //
          make_column("data_type", &metadata_descriptor::data_type_),                   //
          make_column("field_name", &metadata_descriptor::field_name_),                 //
          make_column("choices", &metadata_descriptor::choices_),                       //
          make_column("for_client", &metadata_descriptor::for_client_)
      ),
      make_table(
          "project_status",                                                        //
          make_column("id", &project_status::id_, primary_key().autoincrement()),  //
          make_column("uuid", &project_status::uuid_id_, not_null(), unique()),    //
          make_column("name", &project_status::name_),                             //
          make_column("color", &project_status::color_)
      ),

      make_table(
          "person_department_link", make_column("id", &person_department_link::id_, primary_key().autoincrement()),
          make_column("person_id", &person_department_link::person_id_),
          make_column("department_id", &person_department_link::department_id_),
          foreign_key(&person_department_link::person_id_).references(&person::uuid_id_).on_delete.cascade(),
          foreign_key(&person_department_link::department_id_).references(&department::uuid_id_).on_delete.cascade()
      ),
      make_table(
          "person",                                                                                           //
          make_column("id", &person::id_, primary_key().autoincrement()),                                     //
          make_column("uuid", &person::uuid_id_, not_null(), unique()),                                       //
          make_column("first_name", &person::first_name_),                                                    //
          make_column("last_name", &person::last_name_),                                                      //
          make_column("email", &person::email_),                                                              //
          make_column("phone", &person::phone_),                                                              //
          make_column("contract_type", &person::contract_type_),                                              //
          make_column("active", &person::active_),                                                            //
          make_column("archived", &person::archived_),                                                        //
          make_column("last_presence", &person::last_presence_),                                              //
          make_column("password", &person::password_),                                                        //
          make_column("desktop_login", &person::desktop_login_),                                              //
          make_column("login_failed_attemps", &person::login_failed_attemps_),                                //
          make_column("last_login_failed", &person::last_login_failed_),                                      //
          make_column("totp_enabled", &person::totp_enabled_),                                                //
          make_column("totp_secret", &person::totp_secret_),                                                  //
          make_column("email_otp_enabled", &person::email_otp_enabled_),                                      //
          make_column("email_otp_secret", &person::email_otp_secret_),                                        //
          make_column("fido_enabled", &person::fido_enabled_),                                                //
          make_column("fido_credentials", &person::fido_credentials_),                                        //
          make_column("otp_recovery_codes", &person::otp_recovery_codes_),                                    //
          make_column("preferred_two_factor_authentication", &person::preferred_two_factor_authentication_),  //
          make_column("shotgun_id", &person::shotgun_id_),                                                    //
          make_column("timezone", &person::timezone_),                                                        //
          make_column("locale", &person::locale_),                                                            //
          make_column("data", &person::data_),                                                                //
          make_column("role", &person::role_),                                                                //
          make_column("has_avatar", &person::has_avatar_),                                                    //
          make_column("notifications_enabled", &person::notifications_enabled_),                              //
          make_column("notifications_slack_enabled", &person::notifications_slack_enabled_),                  //
          make_column("notifications_slack_userid", &person::notifications_slack_userid_),                    //
          make_column("notifications_mattermost_enabled", &person::notifications_mattermost_enabled_),        //
          make_column("notifications_mattermost_userid", &person::notifications_mattermost_userid_),          //
          make_column("notifications_discord_enabled", &person::notifications_discord_enabled_),              //
          make_column("notifications_discord_userid", &person::notifications_discord_userid_),                //
          make_column("is_bot", &person::is_bot_),                                                            //
          make_column("jti", &person::jti_),                                                                  //
          make_column("expiration_date", &person::expiration_date_),                                          //
          make_column("studio_id", &person::studio_id_),                                                      //
          make_column("is_generated_from_ldap", &person::is_generated_from_ldap_),                            //
          make_column("ldap_uid", &person::ldap_uid_)
      ),
      make_table(
          "department",                                                        //
          make_column("id", &department::id_, primary_key().autoincrement()),  //
          make_column("uuid", &department::uuid_id_, not_null(), unique()),    //
          make_column("name", &department::name_),                             //
          make_column("color", &department::color_),                           //
          make_column("archived", &department::archived_)                      //
      ),
      make_table(
          "preview_background_file",                                                        //
          make_column("id", &preview_background_file::id_, primary_key().autoincrement()),  //
          make_column("uuid", &preview_background_file::uuid_id_, not_null(), unique()),    //
          make_column("name", &preview_background_file::name_),                             //
          make_column("archived", &preview_background_file::archived_),                     //
          make_column("is_default", &preview_background_file::is_default_),                 //
          make_column("original_name", &preview_background_file::original_name_),           //
          make_column("extension", &preview_background_file::extension_),                   //
          make_column("file_size", &preview_background_file::file_size_)                    //
      ),
      make_table(
          "task_type",                                                        //
          make_column("id", &task_type::id_, primary_key().autoincrement()),  //
          make_column("uuid", &task_type::uuid_id_, not_null(), unique()),    //
          make_column("name", &task_type::name_),                             //
          make_column("short_name", &task_type::short_name_),                 //
          make_column("description", &task_type::description_),               //
          make_column("color", &task_type::color_),                           //
          make_column("priority", &task_type::priority_),                     //
          make_column("for_entity", &task_type::for_entity_),                 //
          make_column("allow_timelog", &task_type::allow_timelog_),           //
          make_column("archived", &task_type::archived_),                     //
          make_column("shotgun_id", &task_type::shotgun_id_)                  //
      ),
      make_table(
          "task_status",                                                           //
          make_column("id", &task_status::id_, primary_key().autoincrement()),     //
          make_column("uuid", &task_status::uuid_id_, not_null(), unique()),       //
          make_column("name", &task_status::name_),                                //
          make_column("archived", &task_status::archived_),                        //
          make_column("short_name", &task_status::short_name_),                    //
          make_column("description", &task_status::description_),                  //
          make_column("color", &task_status::color_),                              //
          make_column("priority", &task_status::priority_),                        //
          make_column("is_done", &task_status::is_done_),                          //
          make_column("is_artist_allowed", &task_status::is_artist_allowed_),      //
          make_column("is_client_allowed", &task_status::is_client_allowed_),      //
          make_column("is_retake", &task_status::is_retake_),                      //
          make_column("is_feedback_request", &task_status::is_feedback_request_),  //
          make_column("is_default", &task_status::is_default_),                    //
          make_column("shotgun_id", &task_status::shotgun_id_),                    //
          make_column("for_concept", &task_status::for_concept_)                   //
      ),
      make_table(
          "asset_type",                                                        //
          make_column("id", &asset_type::id_, primary_key().autoincrement()),  //
          make_column("uuid", &asset_type::uuid_id_, not_null(), unique()),    //
          make_column("name", &asset_type::name_),                             //
          make_column("short_name", &asset_type::short_name_),                 //
          make_column("description", &asset_type::description_),               //
          make_column("archived", &asset_type::archived_)                      //
      ),
      make_table(
          "status_automation",                                                             //
          make_column("id", &status_automation::id_, primary_key().autoincrement()),       //
          make_column("uuid", &status_automation::uuid_id_, not_null(), unique()),         //
          make_column("entity_type", &status_automation::entity_type_),                    //
          make_column("in_task_type_id", &status_automation::in_task_type_id_),            //
          make_column("in_task_status_id", &status_automation::in_task_status_id_),        //
          make_column("out_field_type", &status_automation::out_field_type_),              //
          make_column("out_task_type_id", &status_automation::out_task_type_id_),          //
          make_column("out_task_status_id", &status_automation::out_task_status_id_),      //
          make_column("import_last_revision", &status_automation::import_last_revision_),  //
          make_column("archived", &status_automation::archived_)                           //
      )
  ));
}
using sqlite_orm_type = decltype(make_storage_doodle(""));

constexpr std::size_t g_step_size{500};
}  // namespace
struct sqlite_database_impl {
  using executor_type   = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using strand_type     = boost::asio::strand<boost::asio::io_context::executor_type>;
  using strand_type_ptr = std::shared_ptr<strand_type>;
  strand_type strand_;
  sqlite_orm_type storage_any_;

  explicit sqlite_database_impl(const FSys::path& in_path)
      : strand_(boost::asio::make_strand(g_io_context())),
        storage_any_(std::move(make_storage_doodle(in_path.generic_string()))) {
    storage_any_.open_forever();
    try {
      auto l_g = storage_any_.transaction_guard();
      storage_any_.sync_schema(true);
      l_g.commit();
    } catch (...) {
      default_logger_raw()->error("数据库初始化错误 {}", boost::current_exception_diagnostic_information());
    }
    default_logger_raw()->info("sql thread safe {} ", sqlite_orm::threadsafe());
  }
  std::vector<server_task_info> get_server_task_info(const uuid& in_computer_id) {
    return storage_any_.get_all<server_task_info>(
        sqlite_orm::where(sqlite_orm::c(&server_task_info::run_computer_id_) == in_computer_id)
    );
  }

  std::vector<project_helper::database_t> find_project_by_name(const std::string& in_name) {
    using namespace sqlite_orm;
    return storage_any_.get_all<project_helper::database_t>(
        sqlite_orm::where(sqlite_orm::c(&project_helper::database_t::name_) == in_name)
    );
  }
  std::vector<attendance_helper::database_t> get_attendance(
      const std::int64_t& in_ref_id, const chrono::local_days& in_data
  ) {
    using namespace sqlite_orm;

    return storage_any_.get_all<attendance_helper::database_t>(where(
        c(&attendance_helper::database_t::user_ref) == in_ref_id &&
        c(&attendance_helper::database_t::create_date_) == in_data
    ));
  }

  std::vector<attendance_helper::database_t> get_attendance(
      const std::int64_t& in_ref_id, const std::vector<chrono::local_days>& in_data
  ) {
    using namespace sqlite_orm;
    return storage_any_.get_all<attendance_helper::database_t>(where(
        c(&attendance_helper::database_t::user_ref) == in_ref_id &&
        in(&attendance_helper::database_t::create_date_, in_data)
    ));
  }
  std::vector<work_xlsx_task_info_helper::database_t> get_work_xlsx_task_info(
      const std::int64_t& in_ref_id, const chrono::local_days& in_data
  ) {
    using namespace sqlite_orm;
    return storage_any_.get_all<work_xlsx_task_info_helper::database_t>(where(
        c(&work_xlsx_task_info_helper::database_t::user_ref_) == in_ref_id &&
        c(&work_xlsx_task_info_helper::database_t::year_month_) == in_data
    ));
  }

  std::vector<server_task_info> get_server_task_info_by_user(const uuid& in_user_id) {
    using namespace sqlite_orm;
    return storage_any_.get_all<server_task_info>(where(c(&server_task_info::submitter_) == in_user_id));
  }
  std::vector<server_task_info> get_server_task_info_by_type(const server_task_info_type& in_user_id) {
    using namespace sqlite_orm;
    return storage_any_.get_all<server_task_info>(where(c(&server_task_info::type_) == in_user_id));
  }
#define DOODLE_TO_SQLITE_THREAD()                                 \
  auto this_executor = co_await boost::asio::this_coro::executor; \
  co_await boost::asio::post(boost::asio::bind_executor(strand_, boost::asio::use_awaitable));

  template <typename T>
  std::vector<T> get_all() {
    using namespace sqlite_orm;
    return storage_any_.get_all<T>();
  }

  // template <typename T>
  // std::vector<T> get_all(const uuid& in_uuid) {
  //   using namespace sqlite_orm;
  //   return storage_any_.get_all<T>(sqlite_orm::where(sqlite_orm::c(&T::kitsu_uuid_) == in_uuid));
  // }

  template <typename T>
  std::int64_t uuid_to_id(const uuid& in_uuid) {
    using namespace sqlite_orm;
    auto l_v = storage_any_.select(&T::id_, sqlite_orm::where(sqlite_orm::c(&T::uuid_id_) == in_uuid));
    return l_v.empty() ? 0 : l_v[0];
  }

  template <typename T>
  std::vector<T> get_by_uuid(const uuid& in_uuid) {
    using namespace sqlite_orm;
    return storage_any_.get_all<T>(sqlite_orm::where(sqlite_orm::c(&T::uuid_id_) == in_uuid));
  }

  template <typename T>
  boost::asio::awaitable<void> install(std::shared_ptr<T> in_data) {
    DOODLE_TO_SQLITE_THREAD();

    auto l_g = storage_any_.transaction_guard();
    if (in_data->id_ == 0)
      in_data->id_ = storage_any_.insert<T>(*in_data);
    else {
      storage_any_.replace<T>(*in_data);
    }
    l_g.commit();
    DOODLE_TO_SELF();
  }
  template <typename T>
  boost::asio::awaitable<void> install_range(std::shared_ptr<std::vector<T>> in_data) {
    if (!std::is_sorted(in_data->begin(), in_data->end(), [](const auto& in_r, const auto& in_l) {
          return in_r.id_ < in_l.id_;
        }))
      std::sort(in_data->begin(), in_data->end(), [](const auto& in_r, const auto& in_l) {
        return in_r.id_ < in_l.id_;
      });
    std::size_t l_split =
        std::distance(in_data->begin(), std::ranges::find_if(*in_data, [](const auto& in_) { return in_.id_ != 0; }));

    DOODLE_TO_SQLITE_THREAD();

    auto l_g = storage_any_.transaction_guard();
    for (std::size_t i = 0; i < l_split;) {
      auto l_end = std::min(i + g_step_size, l_split);
      storage_any_.insert_range<T>(in_data->begin() + i, in_data->begin() + l_end);
      i = l_end;
    }

    for (std::size_t i = l_split; i < in_data->size();) {
      auto l_end = std::min(i + g_step_size, in_data->size());
      storage_any_.replace_range<T>(in_data->begin() + i, in_data->begin() + l_end);
      i = l_end;
    }
    l_g.commit();

    for (std::size_t i = 0; i < l_split; ++i) {
      using namespace sqlite_orm;
      auto l_v = storage_any_.select(&T::id_, sqlite_orm::where(c(&T::uuid_id_) == (*in_data)[i].uuid_id_));
      if (!l_v.empty()) (*in_data)[i].id_ = l_v.front();
    }
    DOODLE_TO_SELF();
  }

  template <typename T>
  boost::asio::awaitable<void> remove(std::shared_ptr<std::vector<std::int64_t>> in_data) {
    DOODLE_TO_SQLITE_THREAD();

    auto l_g = storage_any_.transaction_guard();
    storage_any_.remove_all<T>(sqlite_orm::where(sqlite_orm::in(&T::id_, *in_data)));
    l_g.commit();
    DOODLE_TO_SELF();
  }
  template <typename T>
  boost::asio::awaitable<void> remove(std::shared_ptr<uuid> in_data) {
    DOODLE_TO_SQLITE_THREAD();

    auto l_g = storage_any_.transaction_guard();
    storage_any_.remove_all<T>(sqlite_orm::where(sqlite_orm::c(&T::uuid_id_) = *in_data));
    l_g.commit();
    DOODLE_TO_SELF();
  }

  template <typename T>
  std::vector<T> get_by_parent_id(const uuid& in_id) {
    using namespace sqlite_orm;
    return storage_any_.get_all<T>(sqlite_orm::where(sqlite_orm::c(&T::uuid_parent_) == in_id));
  }
};

void sqlite_database::load(const FSys::path& in_path) { impl_ = std::make_shared<sqlite_database_impl>(in_path); }

std::vector<project_helper::database_t> sqlite_database::find_project_by_name(const std::string& in_name) {
  return impl_->find_project_by_name(in_name);
}
std::vector<server_task_info> sqlite_database::get_server_task_info(const uuid& in_computer_id) {
  return impl_->get_server_task_info(in_computer_id);
}

std::vector<attendance_helper::database_t> sqlite_database::get_attendance(
    const std::int64_t& in_ref_id, const chrono::local_days& in_data
) {
  return impl_->get_attendance(in_ref_id, in_data);
}
std::vector<attendance_helper::database_t> sqlite_database::get_attendance(
    const std::int64_t& in_ref_id, const std::vector<chrono::local_days>& in_data
) {
  return impl_->get_attendance(in_ref_id, in_data);
}
std::vector<work_xlsx_task_info_helper::database_t> sqlite_database::get_work_xlsx_task_info(
    const std::int64_t& in_ref_id, const chrono::local_days& in_data
) {
  return impl_->get_work_xlsx_task_info(in_ref_id, in_data);
}
std::vector<server_task_info> sqlite_database::get_server_task_info_by_user(const uuid& in_user_id) {
  return impl_->get_server_task_info_by_user(in_user_id);
}
std::vector<server_task_info> sqlite_database::get_server_task_info_by_type(const server_task_info_type& in_user_id) {
  return impl_->get_server_task_info_by_type(in_user_id);
}

DOODLE_GET_BY_PARENT_ID_SQL(assets_file_helper::database_t);
DOODLE_GET_BY_PARENT_ID_SQL(assets_helper::database_t);

DOODLE_UUID_TO_ID(project_helper::database_t)
DOODLE_UUID_TO_ID(user_helper::database_t)
DOODLE_UUID_TO_ID(metadata::kitsu::task_type_t)
DOODLE_UUID_TO_ID(assets_file_helper::database_t)
DOODLE_UUID_TO_ID(assets_helper::database_t)
DOODLE_UUID_TO_ID(computer)

DOODLE_GET_BY_UUID_SQL(work_xlsx_task_info_helper::database_t)
DOODLE_GET_BY_UUID_SQL(user_helper::database_t)
DOODLE_GET_BY_UUID_SQL(metadata::kitsu::task_type_t)
DOODLE_GET_BY_UUID_SQL(assets_file_helper::database_t)
DOODLE_GET_BY_UUID_SQL(assets_helper::database_t)
DOODLE_GET_BY_UUID_SQL(project_helper::database_t)
DOODLE_GET_BY_UUID_SQL(metadata::kitsu::assets_type_t)
DOODLE_GET_BY_UUID_SQL(computer)
DOODLE_GET_BY_UUID_SQL(server_task_info)

DOODLE_GET_ALL_SQL(project_helper::database_t)
DOODLE_GET_ALL_SQL(user_helper::database_t)
DOODLE_GET_ALL_SQL(metadata::kitsu::task_type_t)
DOODLE_GET_ALL_SQL(assets_file_helper::database_t)
DOODLE_GET_ALL_SQL(metadata::kitsu::assets_type_t)
DOODLE_GET_ALL_SQL(assets_helper::database_t)
DOODLE_GET_ALL_SQL(computer)
DOODLE_GET_ALL_SQL(server_task_info)
DOODLE_GET_ALL_SQL(project_status)
DOODLE_GET_ALL_SQL(task_status)
DOODLE_GET_ALL_SQL(task_type)

DOODLE_INSTALL_SQL(project_helper::database_t)
DOODLE_INSTALL_SQL(user_helper::database_t)
DOODLE_INSTALL_SQL(metadata::kitsu::task_type_t)
DOODLE_INSTALL_SQL(assets_file_helper::database_t)
DOODLE_INSTALL_SQL(assets_helper::database_t)
DOODLE_INSTALL_SQL(computer)
DOODLE_INSTALL_SQL(server_task_info)
DOODLE_INSTALL_SQL(work_xlsx_task_info_helper::database_t)
DOODLE_INSTALL_SQL(project)
DOODLE_INSTALL_SQL(project_status)
DOODLE_INSTALL_SQL(task_status)
DOODLE_INSTALL_SQL(task_type)

DOODLE_INSTALL_RANGE(project_helper::database_t)
DOODLE_INSTALL_RANGE(attendance_helper::database_t)
DOODLE_INSTALL_RANGE(work_xlsx_task_info_helper::database_t)
DOODLE_INSTALL_RANGE(metadata::kitsu::task_type_t)
DOODLE_INSTALL_RANGE(assets_helper::database_t)
DOODLE_INSTALL_RANGE(assets_file_helper::database_t)
DOODLE_INSTALL_RANGE(metadata::kitsu::assets_type_t)
DOODLE_INSTALL_RANGE(computer)
DOODLE_INSTALL_RANGE(task_status)
DOODLE_INSTALL_RANGE(task_type)

DOODLE_REMOVE_RANGE(attendance_helper::database_t)
DOODLE_REMOVE_RANGE(work_xlsx_task_info_helper::database_t)
DOODLE_REMOVE_RANGE(metadata::kitsu::task_type_t)
DOODLE_REMOVE_RANGE(assets_file_helper::database_t)
DOODLE_REMOVE_RANGE(assets_helper::database_t)
DOODLE_REMOVE_RANGE(computer)

DOODLE_REMOVE_BY_UUID(assets_helper::database_t)
DOODLE_REMOVE_BY_UUID(assets_file_helper::database_t)
DOODLE_REMOVE_BY_UUID(computer)
DOODLE_REMOVE_BY_UUID(server_task_info)

}  // namespace doodle