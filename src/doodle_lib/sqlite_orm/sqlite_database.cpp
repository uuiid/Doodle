//
// Created by TD on 24-9-12.
//

#include "sqlite_database.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/attendance.h"
#include "doodle_core/metadata/person.h"
#include "doodle_core/metadata/seedance2/assets_entity_item.h"
#include "doodle_core/metadata/seedance2/group.h"
#include "doodle_core/metadata/seedance2/task.h"
#include <doodle_core/metadata/ai_image_metadata.h>
#include <doodle_core/metadata/asset_instance.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/playlist.h>
#include <doodle_core/metadata/preview_file.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/metadata/working_file.h>

#include "doodle_lib/sqlite_orm/tokenizer/sqlite_jieba.h"
#include <doodle_lib/core/app_base.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>
#include <doodle_lib/sqlite_orm/sqlite_upgrade.h>
#include <doodle_lib/sqlite_orm/tokenizer/sqlite_jieba.h>

#include "core/global_function.h"
#include "orm/fwd.h"
#include "sqlite_orm/orm/column_operations.h"
#include "sqlite_orm/orm/select.h"
#include <cstddef>
#include <optional>
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>

// clang-format off
#include <doodle_lib/sqlite_orm/sqlite_database1.cpp>
#include <doodle_lib/sqlite_orm/sqlite_database2.cpp>
// clang-format on

namespace doodle {

namespace {
template <typename T, typename Attr_Ptr>
auto get_struct_attribute_vector(const std::vector<T>& in, const Attr_Ptr& in_attr)
    -> std::vector<decltype(std::declval<T>().*in_attr)> {
  std::vector<decltype(std::declval<T>().*in_attr)> ret;
  ret.reserve(in.size());
  for (auto&& l_item : in) ret.emplace_back(l_item.*in_attr);
  return ret;
}
template <typename T, typename Attr_Ptr>
auto get_struct_attribute_map(std::vector<T>& in, const Attr_Ptr& in_attr)
    -> std::map<decltype(std::declval<T>().*in_attr), T*> {
  std::map<decltype(std::declval<T>().*in_attr), T*> l_ret{};
  for (auto&& l_item : in) l_ret.emplace(l_item.*in_attr, &l_item);
  return l_ret;
}
}  // namespace

void sqlite_database::regs_all() {
  using namespace orm;
  reg_table<seedance2::assets_entity_item>("seedance2_assets_entity_item")
      .add_column("id", &seedance2::assets_entity_item::id_, primary_key(), autoincrement())
      .add_column("uuid_id", &seedance2::assets_entity_item::uuid_id_, unique(), not_null())
      .add_column("parent_id", &seedance2::assets_entity_item::parent_id_, not_null())
      .add_column("file_extension", &seedance2::assets_entity_item::file_extension_)
      .add_foreign_key(
          "parent_id", &seedance2::assets_entity_item::parent_id_, &seedance2::assets_entity::uuid_id_,
          foreign_key_action::cascade
      );

  reg_table<seedance2::assets_entity>("seedance2_assets_entity")
      .add_column("id", &seedance2::assets_entity::id_, primary_key(), autoincrement())
      .add_column("uuid_id", &seedance2::assets_entity::uuid_id_, unique(), not_null())
      .add_column("name", &seedance2::assets_entity::name_, not_null())
      .add_column("description", &seedance2::assets_entity::description_)
      .add_column("group_id", &seedance2::assets_entity::group_id_, not_null())
      .add_column("user_id", &seedance2::assets_entity::user_id_)
      .add_column("preview_id", &seedance2::assets_entity::preview_id_)
      .add_column("ai_studio_id", &seedance2::assets_entity::ai_studio_id_, not_null())
      .add_column("created_at", &seedance2::assets_entity::created_at_)
      .add_column("updated_at", &seedance2::assets_entity::updated_at_)
      .add_foreign_key(
          "group_id", &seedance2::assets_entity::group_id_, &seedance2::assets_group::uuid_id_,
          foreign_key_action::cascade
      )
      .add_foreign_key("user_id", &seedance2::assets_entity::user_id_, &person::uuid_id_, foreign_key_action::set_null)
      .add_foreign_key(
          "ai_studio_id", &seedance2::assets_entity::ai_studio_id_, &ai_studio::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "preview_id", &seedance2::assets_entity::preview_id_, &seedance2::assets_entity_item::uuid_id_,
          foreign_key_action::set_null
      );

  reg_table<seedance2::assets_group>("seedance2_assets_group")
      .add_column("id", &seedance2::assets_group::id_, primary_key(), autoincrement())
      .add_column("uuid_id", &seedance2::assets_group::uuid_id_, unique(), not_null())
      .add_column("label", &seedance2::assets_group::label_)
      .add_column("user_id", &seedance2::assets_group::user_id_)
      .add_column("ai_studio_id", &seedance2::assets_group::ai_studio_id_)
      .add_column("created_at", &seedance2::assets_group::created_at_);

  reg_table<seedance2::task>("seedance2_task")
      .add_column("id", &seedance2::task::id_, primary_key(), autoincrement())
      .add_column("uuid_id", &seedance2::task::uuid_id_, unique(), not_null())
      .add_column("user_id", &seedance2::task::user_id_)
      .add_column("status", &seedance2::task::status_)
      .add_column("data_request", &seedance2::task::data_request_)
      .add_column("file_extension", &seedance2::task::file_extension_)
      .add_column("data_response", &seedance2::task::data_response_)
      .add_column("ai_studio_id", &seedance2::task::ai_studio_id_)
      .add_column("task_id", &seedance2::task::task_id_)
      .add_column("created_at", &seedance2::task::created_at_)
      .add_column("ended_at", &seedance2::task::ended_at_)
      .add_column("shot_uuid_id", &seedance2::task::shot_uuid_id_)
      .add_column("archived", &seedance2::task::archived_)
      .add_foreign_key("shot_uuid_id", &seedance2::task::shot_uuid_id_, &task::uuid_id_, foreign_key_action::set_null)
      .add_foreign_key("user_id", &seedance2::task::user_id_, &person::uuid_id_, foreign_key_action::set_null)
      .add_index("seedance2_task_uuid_id_index", &seedance2::task::uuid_id_);

  reg_table<ai_studio_person_role_link>("ai_studio_person_role_link")
      .add_column("id", &ai_studio_person_role_link::id_, primary_key(), autoincrement())
      .add_column("ai_studio_id", &ai_studio_person_role_link::ai_studio_id_, not_null())
      .add_column("person_id", &ai_studio_person_role_link::person_id_, not_null())
      .add_foreign_key(
          "ai_studio_id", &ai_studio_person_role_link::ai_studio_id_, &ai_studio::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "person_id", &ai_studio_person_role_link::person_id_, &person::uuid_id_, foreign_key_action::cascade
      );

  reg_table<ai_studio>("ai_studio")
      .add_column("id", &ai_studio::id_, primary_key(), autoincrement())
      .add_column("uuid_id", &ai_studio::uuid_id_, unique(), not_null())
      .add_column("name", &ai_studio::name_, not_null())
      .add_column("color", &ai_studio::color_)
      .add_column("app_key", &ai_studio::app_key_)
      .add_column("app_secret", &ai_studio::app_secret_)
      .add_column("archived", &ai_studio::archived_);

  reg_table<outsource_studio_authorization>("outsource_studio_authorization")
      .add_column("id", &outsource_studio_authorization::id_, primary_key(), autoincrement())
      .add_column("uuid_id", &outsource_studio_authorization::uuid_id_, unique(), not_null())
      .add_column("studio_id", &outsource_studio_authorization::studio_id_, not_null())
      .add_column("entity_id", &outsource_studio_authorization::entity_id_, not_null())
      .add_foreign_key(
          "studio_id", &outsource_studio_authorization::studio_id_, &studio::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "entity_id", &outsource_studio_authorization::entity_id_, &entity::uuid_id_, foreign_key_action::cascade
      )
      .add_unique_index(
          "outsource_studio_authorization_uc", &outsource_studio_authorization::studio_id_,
          &outsource_studio_authorization::entity_id_
      );

  reg_table<ai_image_metadata>("ai_image_metadata")
      .add_column("id", &ai_image_metadata::id_, primary_key(), autoincrement())
      .add_column("uuid_id", &ai_image_metadata::uuid_id_, unique(), not_null())
      .add_column("prompt", &ai_image_metadata::prompt_)
      .add_column("task_id", &ai_image_metadata::task_id_)
      .add_column("category", &ai_image_metadata::category_)
      .add_column("extension", &ai_image_metadata::extension_)
      .add_column("width", &ai_image_metadata::width_)
      .add_column("height", &ai_image_metadata::height_)
      .add_column("created_at", &ai_image_metadata::created_at_)
      .add_column("author", &ai_image_metadata::author_)
      .add_foreign_key("author", &ai_image_metadata::author_, &person::uuid_id_, foreign_key_action::cascade);

  reg_table<playlist_shot>("playlist_shot")
      .add_column("id", &playlist_shot::id_, primary_key(), autoincrement())
      .add_column("uuid_id", &playlist_shot::uuid_id_, unique(), not_null())
      .add_column("playlist_id", &playlist_shot::playlist_id_, not_null())
      .add_column("entity_id", &playlist_shot::entity_id_, not_null())
      .add_column("preview_id", &playlist_shot::preview_id_)
      .add_column("order_index", &playlist_shot::order_index_)
      .add_foreign_key("playlist_id", &playlist_shot::playlist_id_, &playlist::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("entity_id", &playlist_shot::entity_id_, &entity::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("preview_id", &playlist_shot::preview_id_, &preview_file::uuid_id_, foreign_key_action::cascade)
      .add_unique_index(
          "playlist_shot_uc", &playlist_shot::playlist_id_, &playlist_shot::entity_id_, &playlist_shot::preview_id_
      );

  reg_table<playlist>("playlist")
      .add_column("id", &playlist::id_, primary_key(), autoincrement())
      .add_column("uuid_id", &playlist::uuid_id_, unique(), not_null())
      .add_column("name", &playlist::name_, not_null())
      .add_column("project_id", &playlist::project_id_)
      .add_column("episode_id", &playlist::episodes_id_)
      .add_column("task_type_id", &playlist::task_type_id_)
      .add_column("for_client", &playlist::for_client_)
      .add_column("for_entity", &playlist::for_entity_)
      .add_column("is_for_all", &playlist::is_for_all_)
      .add_column("created_at", &playlist::created_at_)
      .add_column("updated_at", &playlist::updated_at_)
      .add_foreign_key("project_id", &playlist::project_id_, &project::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("episode_id", &playlist::episodes_id_, &entity::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("task_type_id", &playlist::task_type_id_, &task_type::uuid_id_, foreign_key_action::cascade)
      .add_unique_index("playlist_uc", &playlist::name_, &playlist::project_id_, &playlist::episodes_id_);

  reg_table<server_task_info>("server_task_info_tab")
      .add_column("id", &server_task_info::id_, primary_key())
      .add_column("uuid_id", &server_task_info::uuid_id_, unique(), not_null())
      .add_column("command", &server_task_info::command_)
      .add_column("status", &server_task_info::status_)
      .add_column("name", &server_task_info::name_)
      .add_column("source_computer", &server_task_info::source_computer_)
      .add_column("submitter", &server_task_info::submitter_)
      .add_column("submit_time", &server_task_info::submit_time_)
      .add_column("priority", &server_task_info::priority_)
      .add_column("run_time", &server_task_info::run_time_)
      .add_column("end_time", &server_task_info::end_time_)
      .add_column("run_computer_id", &server_task_info::run_computer_id_)
      .add_column("task_id", &server_task_info::task_id_)
      .add_column("type", &server_task_info::type_)
      .add_column("run_time_info", &server_task_info::run_time_info_)
      .add_foreign_key("submitter", &server_task_info::submitter_, &person::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key(
          "run_computer_id", &server_task_info::run_computer_id_, &computer::uuid_id_, foreign_key_action::set_null
      )
      .add_foreign_key("task_id", &server_task_info::task_id_, &task::uuid_id_, foreign_key_action::cascade)
      .add_index("server_task_info_tab_uuid_id_idx", &server_task_info::uuid_id_);

  reg_table<computer>("computer")
      .add_column("id", &computer::id_, primary_key())
      .add_column("uuid_id", &computer::uuid_id_, unique(), not_null())
      .add_column("hardware_id", &computer::hardware_id_, unique(), not_null())
      .add_column("name", &computer::name_, not_null())
      .add_column("status", &computer::status_)
      .add_column("last_heartbeat_time", &computer::last_heartbeat_time_)
      .add_column("bot_uuid", &computer::bot_uuid_)
      .add_foreign_key("bot_uuid", &computer::bot_uuid_, &person::uuid_id_, foreign_key_action::cascade);

  reg_table<assets_file_helper::link_parent_t>("assets_link_parent_t")
      .add_column("id", &assets_file_helper::link_parent_t::id_, primary_key())
      .add_column("assets_type_uuid", &assets_file_helper::link_parent_t::assets_type_uuid_, not_null())
      .add_column("assets_uuid", &assets_file_helper::link_parent_t::assets_uuid_, not_null())
      .add_foreign_key(
          "assets_type_uuid", &assets_file_helper::link_parent_t::assets_type_uuid_,
          &assets_helper::database_t::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "assets_uuid", &assets_file_helper::link_parent_t::assets_uuid_, &assets_file_helper::database_t::uuid_id_,
          foreign_key_action::cascade
      );

  reg_table<assets_file_helper::database_t>("assets_file_tab_2")
      .add_column("id", &assets_file_helper::database_t::id_, primary_key())
      .add_column("uuid_id", &assets_file_helper::database_t::uuid_id_, unique(), not_null())
      .add_column("label", &assets_file_helper::database_t::label_)
      .add_column("path", &assets_file_helper::database_t::path_)
      .add_column("notes", &assets_file_helper::database_t::notes_)
      .add_column("active", &assets_file_helper::database_t::active_)
      .add_column("has_thumbnail", &assets_file_helper::database_t::has_thumbnail_, default_value("FALSE"s))
      .add_column("extension", &assets_file_helper::database_t::extension_, default_value(".png"s));

  reg_table<assets_helper::database_t>("assets_tab")
      .add_column("id", &assets_helper::database_t::id_, primary_key())
      .add_column("uuid_id", &assets_helper::database_t::uuid_id_, unique(), not_null())
      .add_column("label", &assets_helper::database_t::label_, not_null())
      .add_column("parent_uuid", &assets_helper::database_t::uuid_parent_)
      .add_column("order", &assets_helper::database_t::order_, default_value("0"s), not_null())
      .add_index("assets_tab_label", &assets_helper::database_t::label_);

  reg_table<attendance_helper::database_t>("attendance_tab")
      .add_column("id", &attendance_helper::database_t::id_, primary_key())
      .add_column("uuid_id", &attendance_helper::database_t::uuid_id_, unique(), not_null())
      .add_column("start_time", &attendance_helper::database_t::start_time_)
      .add_column("end_time", &attendance_helper::database_t::end_time_)
      .add_column("remark", &attendance_helper::database_t::remark_)
      .add_column("att_enum", &attendance_helper::database_t::type_)
      .add_column("create_date", &attendance_helper::database_t::create_date_)
      .add_column("update_time", &attendance_helper::database_t::update_time_)
      .add_column("dingding_id", &attendance_helper::database_t::dingding_id_)
      .add_column("person_id", &attendance_helper::database_t::person_id_)
      .add_foreign_key(
          "person_id", &attendance_helper::database_t::person_id_, &person::uuid_id_, foreign_key_action::cascade
      )
      .add_index("attendance_tab_uuid_id_index", &attendance_helper::database_t::uuid_id_)
      .add_index("attendance_tab_create_date_index", &attendance_helper::database_t::create_date_);

  reg_table<work_xlsx_task_info_helper::database_t>("work_xlsx_task_info_tab")
      .add_column("id", &work_xlsx_task_info_helper::database_t::id_, primary_key())
      .add_column("uuid_id", &work_xlsx_task_info_helper::database_t::uuid_id_, unique(), not_null())
      .add_column("start_time", &work_xlsx_task_info_helper::database_t::start_time_)
      .add_column("end_time", &work_xlsx_task_info_helper::database_t::end_time_)
      .add_column("duration", &work_xlsx_task_info_helper::database_t::duration_)
      .add_column("remark", &work_xlsx_task_info_helper::database_t::remark_)
      .add_column("user_remark", &work_xlsx_task_info_helper::database_t::user_remark_)
      .add_column("year_month", &work_xlsx_task_info_helper::database_t::year_month_)
      .add_column("person_id", &work_xlsx_task_info_helper::database_t::person_id_)
      .add_column("kitsu_task_ref_id", &work_xlsx_task_info_helper::database_t::kitsu_task_ref_id_)
      .add_column("season", &work_xlsx_task_info_helper::database_t::season_)
      .add_column("episode", &work_xlsx_task_info_helper::database_t::episode_)
      .add_column("name", &work_xlsx_task_info_helper::database_t::name_)
      .add_column("grade", &work_xlsx_task_info_helper::database_t::grade_)
      .add_column("project_id", &work_xlsx_task_info_helper::database_t::project_id_)
      .add_column("project_name", &work_xlsx_task_info_helper::database_t::project_name_)
      .add_foreign_key(
          "person_id", &work_xlsx_task_info_helper::database_t::person_id_, &person::uuid_id_,
          foreign_key_action::cascade
      )
      .add_index("work_xlsx_task_info_tab_year_month_index", &work_xlsx_task_info_helper::database_t::year_month_);

  reg_table<attachment_file>("attachment_file")
      .add_column("id", &attachment_file::id_, primary_key(), autoincrement())
      .add_column("uuid", &attachment_file::uuid_id_, unique(), not_null())
      .add_column("name", &attachment_file::name_)
      .add_column("size", &attachment_file::size_)
      .add_column("extension", &attachment_file::extension_)
      .add_column("mimetype", &attachment_file::mimetype_)
      .add_column("comment_id", &attachment_file::comment_id_)
      .add_column("chat_message_id", &attachment_file::chat_message_id_)
      .add_foreign_key("comment_id", &attachment_file::comment_id_, &comment::uuid_id_, foreign_key_action::cascade)
      .add_index("attachment_file_chat_message_id_index", &attachment_file::chat_message_id_);

  reg_table<subscription>("subscription")
      .add_column("id", &subscription::id_, primary_key(), autoincrement())
      .add_column("uuid", &subscription::uuid_id_, unique(), not_null())
      .add_column("person_id", &subscription::person_id_, not_null())
      .add_column("task_id", &subscription::task_id_)
      .add_column("entity_id", &subscription::entity_id_)
      .add_column("task_type_id", &subscription::task_type_id_)
      .add_foreign_key("person_id", &subscription::person_id_, &person::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("task_id", &subscription::task_id_, &task::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("entity_id", &subscription::entity_id_, &entity::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("task_type_id", &subscription::task_type_id_, &task_type::uuid_id_, foreign_key_action::cascade)
      .add_unique_index(
          "subscription_entity_uc", &subscription::person_id_, &subscription::task_type_id_, &subscription::entity_id_
      )
      .add_unique_index("subscription_task_uc", &subscription::person_id_, &subscription::task_id_);

  reg_table<assignees_table>("assignations")
      .add_column("id", &assignees_table::id_, primary_key(), autoincrement())
      .add_column("person_id", &assignees_table::person_id_, not_null())
      .add_column("task_id", &assignees_table::task_id_, not_null())
      .add_foreign_key("person_id", &assignees_table::person_id_, &person::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("task_id", &assignees_table::task_id_, &task::uuid_id_, foreign_key_action::cascade);

  reg_table<comment_preview_link>("comment_preview_link")
      .add_column("id", &comment_preview_link::id_, primary_key(), autoincrement())
      .add_column("comment_id", &comment_preview_link::comment_id_)
      .add_column("preview_file_id", &comment_preview_link::preview_file_id_)
      .add_foreign_key(
          "comment_id", &comment_preview_link::comment_id_, &comment::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "preview_file_id", &comment_preview_link::preview_file_id_, &preview_file::uuid_id_,
          foreign_key_action::cascade
      );

  reg_table<preview_file>("preview_file")
      .add_column("id", &preview_file::id_, primary_key(), autoincrement())
      .add_column("uuid", &preview_file::uuid_id_, unique(), not_null())
      .add_column("name", &preview_file::name_, unique())
      .add_column("original_name", &preview_file::original_name_)
      .add_column("revision", &preview_file::revision_)
      .add_column("position", &preview_file::position_)
      .add_column("extension", &preview_file::extension_)
      .add_column("description", &preview_file::description_)
      .add_column("path", &preview_file::path_)
      .add_column("source", &preview_file::source_)
      .add_column("file_size", &preview_file::file_size_)
      .add_column("status", &preview_file::status_)
      .add_column("validation_status", &preview_file::validation_status_)
      .add_column("annotations", &preview_file::annotations_)
      .add_column("width", &preview_file::width_)
      .add_column("height", &preview_file::height_)
      .add_column("duration", &preview_file::duration_)
      .add_column("task_id", &preview_file::task_id_)
      .add_column("shotgun_id", &preview_file::shotgun_id_)
      .add_column("person_id", &preview_file::person_id_)
      .add_column("source_file_id", &preview_file::source_file_id_)
      .add_column("is_movie", &preview_file::is_movie_)
      .add_column("url", &preview_file::url_)
      .add_column("uploaded_movie_url", &preview_file::uploaded_movie_url_)
      .add_column("uploaded_movie_name", &preview_file::uploaded_movie_name_)
      .add_column("created_at", &preview_file::created_at_)
      .add_column("updated_at", &preview_file::updated_at_)
      .add_foreign_key("task_id", &preview_file::task_id_, &task::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("person_id", &preview_file::person_id_, &person::uuid_id_, foreign_key_action::set_null)
      .add_unique_index("preview_file_uc", &preview_file::name_, &preview_file::task_id_, &preview_file::revision_);

  reg_table<notification>("notification_2")
      .add_column("id", &notification::id_, primary_key(), autoincrement())
      .add_column("uuid", &notification::uuid_id_, unique(), not_null())
      .add_column("read", &notification::read_)
      .add_column("change", &notification::change_)
      .add_column("type", &notification::type_)
      .add_column("person_id", &notification::person_id_, not_null())
      .add_column("author_id", &notification::author_id_, not_null())
      .add_column("comment_id", &notification::comment_id_)
      .add_column("task_id", &notification::task_id_, not_null())
      .add_column("reply_id", &notification::reply_id_)
      .add_column("created_at", &notification::created_at_)
      .add_foreign_key("person_id", &notification::person_id_, &person::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("author_id", &notification::author_id_, &person::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("comment_id", &notification::comment_id_, &comment::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("task_id", &notification::task_id_, &task::uuid_id_, foreign_key_action::cascade);

  reg_table<comment_mentions>("comment_mentions")
      .add_column("id", &comment_mentions::id_, primary_key(), autoincrement())
      .add_column("comment_id", &comment_mentions::comment_id_)
      .add_column("person_id", &comment_mentions::person_id_)
      .add_foreign_key("comment_id", &comment_mentions::comment_id_, &comment::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("person_id", &comment_mentions::person_id_, &person::uuid_id_, foreign_key_action::cascade);

  reg_table<comment_department_mentions>("comment_department_mentions")
      .add_column("id", &comment_department_mentions::id_, primary_key(), autoincrement())
      .add_column("comment_id", &comment_department_mentions::comment_id_)
      .add_column("department_id", &comment_department_mentions::department_id_)
      .add_foreign_key(
          "comment_id", &comment_department_mentions::comment_id_, &comment::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "department_id", &comment_department_mentions::department_id_, &department::uuid_id_,
          foreign_key_action::cascade
      );

  reg_table<comment_acknoledgments>("comment_acknoledgments")
      .add_column("id", &comment_acknoledgments::id_, primary_key(), autoincrement())
      .add_column("comment_id", &comment_acknoledgments::comment_id_)
      .add_column("person_id", &comment_acknoledgments::person_id_)
      .add_foreign_key(
          "comment_id", &comment_acknoledgments::comment_id_, &comment::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "person_id", &comment_acknoledgments::person_id_, &person::uuid_id_, foreign_key_action::cascade
      );

  reg_table<comment>("comment")
      .add_column("id", &comment::id_, primary_key(), autoincrement())
      .add_column("uuid", &comment::uuid_id_, unique(), not_null())
      .add_column("shotgun_id", &comment::shotgun_id_)
      .add_column("object_id", &comment::object_id_, not_null())
      .add_column("object_type", &comment::object_type_, not_null())
      .add_column("text", &comment::text_)
      .add_column("data", &comment::data_)
      .add_column("replies", &comment::replies_)
      .add_column("checklist", &comment::checklist_)
      .add_column("pinned", &comment::pinned_)
      .add_column("links", &comment::links)
      .add_column("created_at", &comment::created_at_)
      .add_column("updated_at", &comment::updated_at_)
      .add_column("task_status_id", &comment::task_status_id_)
      .add_column("person_id", &comment::person_id_, not_null())
      .add_column("editor_id", &comment::editor_id_)
      .add_column("preview_file_id", &comment::preview_file_id_)
      .add_foreign_key("task_status_id", &comment::task_status_id_, &task_status::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("person_id", &comment::person_id_, &person::uuid_id_, foreign_key_action::set_null)
      .add_foreign_key("editor_id", &comment::editor_id_, &person::uuid_id_, foreign_key_action::set_null)
      .add_foreign_key(
          "preview_file_id", &comment::preview_file_id_, &preview_file::uuid_id_, foreign_key_action::set_null
      )
      .add_foreign_key(
          "comment_object_id_object_type_fk", &comment::object_id_, &entity::uuid_id_, foreign_key_action::cascade
      )
      .add_index("comment_object_type_index", &comment::object_type_);

  reg_table<task>("task")
      .add_column("id", &task::id_, primary_key(), autoincrement())
      .add_column("uuid", &task::uuid_id_, unique(), not_null())
      .add_column("name", &task::name_)
      .add_column("description", &task::description_)
      .add_column("priority", &task::priority_)
      .add_column("difficulty", &task::difficulty_)
      .add_column("duration", &task::duration_)
      .add_column("estimation", &task::estimation_)
      .add_column("completion_rate", &task::completion_rate_)
      .add_column("retake_count", &task::retake_count_)
      .add_column("sort_order", &task::sort_order_)
      .add_column("start_date", &task::start_date_)
      .add_column("due_date", &task::due_date_)
      .add_column("real_start_date", &task::real_start_date_)
      .add_column("end_date", &task::end_date_)
      .add_column("done_date", &task::done_date_)
      .add_column("last_comment_date", &task::last_comment_date_)
      .add_column("nb_assets_ready", &task::nb_assets_ready_)
      .add_column("data", &task::data_)
      .add_column("shotgun_id", &task::shotgun_id_)
      .add_column("last_preview_file_id", &task::last_preview_file_id_)
      .add_column("nb_drawings", &task::nb_drawings_)
      .add_column("created_at", &task::created_at_)
      .add_column("updated_at", &task::updated_at_)
      .add_column("project_id", &task::project_id_)
      .add_column("task_type_id", &task::task_type_id_)
      .add_column("task_status_id", &task::task_status_id_)
      .add_column("entity_id", &task::entity_id_)
      .add_column("assigner_id", &task::assigner_id_)
      .add_foreign_key("project_id", &task::project_id_, &project::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("task_type_id", &task::task_type_id_, &task_type::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("task_status_id", &task::task_status_id_, &task_status::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("entity_id", &task::entity_id_, &entity::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("assigner_id", &task::assigner_id_, &person::uuid_id_, foreign_key_action::set_null)
      .add_unique_index("task_uc", &task::name_, &task::project_id_, &task::task_type_id_, &task::entity_id_);

  reg_table<entity_link>("entity_link")
      .add_column("id", &entity_link::id_, primary_key(), autoincrement())
      .add_column("uuid", &entity_link::uuid_id_, unique(), not_null())
      .add_column("entity_in_id", &entity_link::entity_in_id_)
      .add_column("entity_out_id", &entity_link::entity_out_id_)
      .add_column("data", &entity_link::data_)
      .add_column("nb_occurences", &entity_link::nb_occurences_)
      .add_column("label", &entity_link::label_)
      .add_foreign_key("entity_in_id", &entity_link::entity_in_id_, &entity::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("entity_out_id", &entity_link::entity_out_id_, &entity::uuid_id_, foreign_key_action::cascade);

  reg_table<entity_concept_link>("entity_concept_link")
      .add_column("id", &entity_concept_link::id_, primary_key(), autoincrement())
      .add_column("entity_in_id", &entity_concept_link::entity_id_)
      .add_column("entity_out_id", &entity_concept_link::entity_out_id_)
      .add_foreign_key("entity_in_id", &entity_concept_link::entity_id_, &entity::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key(
          "entity_out_id", &entity_concept_link::entity_out_id_, &entity::uuid_id_, foreign_key_action::cascade
      );

  reg_table<entity_shot_extend>("entity_shot_extend")
      .add_column("id", &entity_shot_extend::id_, primary_key(), autoincrement())
      .add_column("uuid", &entity_shot_extend::uuid_id_, unique(), not_null())
      .add_column("entity_id", &entity_shot_extend::entity_id_)
      .add_column("frame_in", &entity_shot_extend::frame_in_)
      .add_column("frame_out", &entity_shot_extend::frame_out_)
      .add_foreign_key("entity_id", &entity_shot_extend::entity_id_, &entity::uuid_id_, foreign_key_action::cascade);

  reg_table<entity_asset_extend>("entity_asset_extend")
      .add_column("id", &entity_asset_extend::id_, primary_key(), autoincrement())
      .add_column("uuid", &entity_asset_extend::uuid_id_, unique(), not_null())
      .add_column("entity_id", &entity_asset_extend::entity_id_, not_null())
      .add_column("ji_shu_lie", &entity_asset_extend::ji_shu_lie_)
      .add_column("deng_ji", &entity_asset_extend::deng_ji_)
      .add_column("gui_dang", &entity_asset_extend::gui_dang_)
      .add_column("bian_hao", &entity_asset_extend::bian_hao_)
      .add_column("pin_yin_ming_cheng", &entity_asset_extend::pin_yin_ming_cheng_)
      .add_column("ban_ben", &entity_asset_extend::ban_ben_)
      .add_column("ji_du", &entity_asset_extend::ji_du_)
      .add_column("kai_shi_ji_shu", &entity_asset_extend::kai_shi_ji_shu_)
      .add_column("chang_ci", &entity_asset_extend::chang_ci_)
      .add_foreign_key("entity_id", &entity_asset_extend::entity_id_, &entity::uuid_id_, foreign_key_action::cascade);

  reg_table<entity>("entity")
      .add_column("id", &entity::id_, primary_key(), autoincrement())
      .add_column("uuid", &entity::uuid_id_, unique(), not_null())
      .add_column("name", &entity::name_)
      .add_column("code", &entity::code_)
      .add_column("description", &entity::description_)
      .add_column("shotgun_id", &entity::shotgun_id_)
      .add_column("canceled", &entity::canceled_)
      .add_column("nb_frames", &entity::nb_frames_)
      .add_column("nb_entities_out", &entity::nb_entities_out_)
      .add_column("is_casting_standby", &entity::is_casting_standby_)
      .add_column("is_shared", &entity::is_shared_)
      .add_column("status", &entity::status_)
      .add_column("project_id", &entity::project_id_, not_null())
      .add_column("entity_type_id", &entity::entity_type_id_, not_null())
      .add_column("parent_id", &entity::parent_id_)
      .add_column("source_id", &entity::source_id_)
      .add_column("preview_file_id", &entity::preview_file_id_)
      .add_column("ready_for", &entity::ready_for_)
      .add_column("created_by", &entity::created_by_)
      .add_foreign_key("project_id", &entity::project_id_, &project::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("entity_type_id", &entity::entity_type_id_, &asset_type::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key(
          "preview_file_id", &entity::preview_file_id_, &preview_file::uuid_id_, foreign_key_action::set_null
      )
      .add_foreign_key("ready_for", &entity::ready_for_, &task_type::uuid_id_, foreign_key_action::set_null)
      .add_foreign_key("created_by", &entity::created_by_, &person::uuid_id_, foreign_key_action::set_null)
      .add_foreign_key("parent_id", &entity::parent_id_, &entity::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("source_id", &entity::source_id_, &entity::uuid_id_, foreign_key_action::cascade)
      .add_unique_index(
          "entity_uc", &entity::name_, &entity::project_id_, &entity::entity_type_id_, &entity::parent_id_
      );

  reg_table<task_type_asset_type_link>("task_type_asset_type_link")
      .add_column("id", &task_type_asset_type_link::id_, primary_key(), autoincrement())
      .add_column("asset_type_id", &task_type_asset_type_link::asset_type_id_, not_null())
      .add_column("task_type_id", &task_type_asset_type_link::task_type_id_, not_null())
      .add_foreign_key(
          "asset_type_id", &task_type_asset_type_link::asset_type_id_, &asset_type::uuid_id_,
          foreign_key_action::cascade
      )
      .add_foreign_key(
          "task_type_id", &task_type_asset_type_link::task_type_id_, &task_type::uuid_id_, foreign_key_action::cascade
      )
      .add_unique_index(
          "task_type_asset_type_link_uc", &task_type_asset_type_link::task_type_id_,
          &task_type_asset_type_link::asset_type_id_
      );

  reg_table<project_person_link>("project_person_link")
      .add_column("id", &project_person_link::id_, primary_key(), autoincrement())
      .add_column("project_id", &project_person_link::project_id_, not_null())
      .add_column("person_id", &project_person_link::person_id_, not_null())
      .add_column("shotgun_id", &project_person_link::shotgun_id_)
      .add_foreign_key("project_id", &project_person_link::project_id_, &project::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key("person_id", &project_person_link::person_id_, &person::uuid_id_, foreign_key_action::cascade);

  reg_table<project_task_type_link>("project_task_type_link")
      .add_column("id", &project_task_type_link::id_, primary_key(), autoincrement())
      .add_column("uuid", &project_task_type_link::uuid_id_, unique(), not_null())
      .add_column("project_id", &project_task_type_link::project_id_, not_null())
      .add_column("task_type_id", &project_task_type_link::task_type_id_, not_null())
      .add_column("priority", &project_task_type_link::priority_)
      .add_foreign_key(
          "project_id", &project_task_type_link::project_id_, &project::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "task_type_id", &project_task_type_link::task_type_id_, &task_type::uuid_id_, foreign_key_action::cascade
      )
      .add_unique_index(
          "project_task_type_link_uc", &project_task_type_link::project_id_, &project_task_type_link::task_type_id_
      );

  reg_table<project_task_status_link>("project_task_status_link")
      .add_column("id", &project_task_status_link::id_, primary_key(), autoincrement())
      .add_column("uuid", &project_task_status_link::uuid_id_, unique(), not_null())
      .add_column("project_id", &project_task_status_link::project_id_, not_null())
      .add_column("task_status_id", &project_task_status_link::task_status_id_, not_null())
      .add_column("priority", &project_task_status_link::priority_)
      .add_column("roles_for_board", &project_task_status_link::roles_for_board_)
      .add_foreign_key(
          "project_id", &project_task_status_link::project_id_, &project::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "task_status_id", &project_task_status_link::task_status_id_, &task_status::uuid_id_,
          foreign_key_action::cascade
      )
      .add_unique_index(
          "project_task_status_link_uc", &project_task_status_link::project_id_,
          &project_task_status_link::task_status_id_
      );

  reg_table<project_asset_type_link>("project_asset_type_link")
      .add_column("id", &project_asset_type_link::id_, primary_key(), autoincrement())
      .add_column("project_id", &project_asset_type_link::project_id_, not_null())
      .add_column("asset_type_id", &project_asset_type_link::asset_type_id_, not_null())
      .add_foreign_key(
          "project_id", &project_asset_type_link::project_id_, &project::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "asset_type_id", &project_asset_type_link::asset_type_id_, &asset_type::uuid_id_, foreign_key_action::cascade
      );

  reg_table<project_status_automation_link>("project_status_automation_link")
      .add_column("id", &project_status_automation_link::id_, primary_key(), autoincrement())
      .add_column("project_id", &project_status_automation_link::project_id_, not_null())
      .add_column("status_automation_id", &project_status_automation_link::status_automation_id_, not_null())
      .add_foreign_key(
          "project_id", &project_status_automation_link::project_id_, &project::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "status_automation_id", &project_status_automation_link::status_automation_id_, &status_automation::uuid_id_,
          foreign_key_action::cascade
      );

  reg_table<project_preview_background_file_link>("project_preview_background_file_link")
      .add_column("id", &project_preview_background_file_link::id_, primary_key(), autoincrement())
      .add_column("project_id", &project_preview_background_file_link::project_id_, not_null())
      .add_column(
          "preview_background_file_id", &project_preview_background_file_link::preview_background_file_id_, not_null()
      )
      .add_foreign_key(
          "project_id", &project_preview_background_file_link::project_id_, &project::uuid_id_,
          foreign_key_action::cascade
      )
      .add_foreign_key(
          "preview_background_file_id", &project_preview_background_file_link::preview_background_file_id_,
          &preview_background_file::uuid_id_, foreign_key_action::cascade
      );

  reg_table<project>("project")
      .add_column("id", &project::id_, primary_key(), autoincrement())
      .add_column("uuid", &project::uuid_id_, not_null(), unique())
      .add_column("name", &project::name_, not_null())
      .add_column("code", &project::code_)
      .add_column("description", &project::description_)
      .add_column("shotgun_id", &project::shotgun_id_)
      .add_column("file_tree", &project::file_tree_)
      .add_column("data", &project::data_)
      .add_column("has_avatar", &project::has_avatar_)
      .add_column("fps", &project::fps_)
      .add_column("ratio", &project::ratio_)
      .add_column("resolution", &project::resolution_)
      .add_column("production_type", &project::production_type_)
      .add_column("production_style", &project::production_style_)
      .add_column("start_date", &project::start_date_)
      .add_column("end_date", &project::end_date_)
      .add_column("man_days", &project::man_days_)
      .add_column("nb_episodes", &project::nb_episodes_)
      .add_column("episode_span", &project::episode_span_)
      .add_column("max_retakes", &project::max_retakes_)
      .add_column("is_clients_isolated", &project::is_clients_isolated_)
      .add_column("is_preview_download_allowed", &project::is_preview_download_allowed_)
      .add_column("is_set_preview_automated", &project::is_set_preview_automated_)
      .add_column("homepage", &project::homepage_)
      .add_column("is_publish_default_for_artists", &project::is_publish_default_for_artists_)
      .add_column("hd_bitrate_compression", &project::hd_bitrate_compression_)
      .add_column("ld_bitrate_compression", &project::ld_bitrate_compression_)
      .add_column("project_status_id", &project::project_status_id_)
      .add_column("default_preview_background_file_id", &project::default_preview_background_file_id_)
      .add_column("path", &project::path_)
      .add_column("en_str", &project::en_str_)
      .add_column("auto_upload_path", &project::auto_upload_path_)
      .add_column("production_category", &project::production_category_)
      .add_column("short_name", &project::short_name_)
      .add_column("asset_root_path", &project::asset_root_path_, default_value(""))
      .add_foreign_key(
          "project_status_id", &project::project_status_id_, &project_status::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "default_preview_background_file_id", &project::default_preview_background_file_id_,
          &preview_background_file::uuid_id_, foreign_key_action::set_null
      );

  reg_table<metadata_descriptor_department_link>("metadata_descriptor_department_link")
      .add_column("id", &metadata_descriptor_department_link::id_, primary_key(), autoincrement())
      .add_column("metadata_descriptor_id", &metadata_descriptor_department_link::metadata_descriptor_uuid_)
      .add_column("department_id", &metadata_descriptor_department_link::department_uuid_)
      .add_foreign_key(
          "metadata_descriptor_id", &metadata_descriptor_department_link::metadata_descriptor_uuid_,
          &metadata_descriptor::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "department_id", &metadata_descriptor_department_link::department_uuid_, &department::uuid_id_,
          foreign_key_action::cascade
      );

  reg_table<metadata_descriptor>("metadata_descriptor")
      .add_column("id", &metadata_descriptor::id_, primary_key(), autoincrement())
      .add_column("uuid", &metadata_descriptor::uuid_id_, not_null(), unique())
      .add_column("name", &metadata_descriptor::name_, not_null())
      .add_column("entity_type", &metadata_descriptor::entity_type_, not_null())
      .add_column("project_id", &metadata_descriptor::project_uuid_, not_null())
      .add_column("data_type", &metadata_descriptor::data_type_, not_null())
      .add_column("field_name", &metadata_descriptor::field_name_, not_null())
      .add_column("choices", &metadata_descriptor::choices_)
      .add_column("for_client", &metadata_descriptor::for_client_);

  reg_table<project_status>("project_status")
      .add_column("id", &project_status::id_, primary_key(), autoincrement())
      .add_column("uuid", &project_status::uuid_id_, not_null(), unique())
      .add_column("name", &project_status::name_, not_null(), unique())
      .add_column("color", &project_status::color_, not_null());

  reg_table<person_department_link>("department_link")
      .add_column("id", &person_department_link::id_, primary_key(), autoincrement())
      .add_column("person_id", &person_department_link::person_id_)
      .add_column("department_id", &person_department_link::department_id_)
      .add_foreign_key("person_id", &person_department_link::person_id_, &person::uuid_id_, foreign_key_action::cascade)
      .add_foreign_key(
          "department_id", &person_department_link::department_id_, &department::uuid_id_, foreign_key_action::cascade
      )

      .add_unique_index(
          "department_link_uc", &person_department_link::person_id_, &person_department_link::department_id_
      );

  reg_table<person>("person")
      .add_column("id", &person::id_, primary_key(), autoincrement())
      .add_column("uuid", &person::uuid_id_, not_null(), unique())
      .add_column("first_name", &person::first_name_)
      .add_column("last_name", &person::last_name_)
      .add_column("email", &person::email_)
      .add_column("phone", &person::phone_)
      .add_column("contract_type", &person::contract_type_)
      .add_column("active", &person::active_)
      .add_column("archived", &person::archived_)
      .add_column("last_presence", &person::last_presence_)
      .add_column("password", &person::password_)
      .add_column("desktop_login", &person::desktop_login_)
      .add_column("login_failed_attemps", &person::login_failed_attemps_)
      .add_column("last_login_failed", &person::last_login_failed_)
      .add_column("totp_enabled", &person::totp_enabled_)
      .add_column("totp_secret", &person::totp_secret_)
      .add_column("email_otp_enabled", &person::email_otp_enabled_)
      .add_column("email_otp_secret", &person::email_otp_secret_)
      .add_column("fido_enabled", &person::fido_enabled_)
      .add_column("fido_credentials", &person::fido_credentials_)
      .add_column("otp_recovery_codes", &person::otp_recovery_codes_)
      .add_column("preferred_two_factor_authentication", &person::preferred_two_factor_authentication_)
      .add_column("shotgun_id", &person::shotgun_id_, unique())
      .add_column("timezone", &person::timezone_)
      .add_column("locale", &person::locale_)
      .add_column("data", &person::data_)
      .add_column("role", &person::role_)
      .add_column("has_avatar", &person::has_avatar_)
      .add_column("notifications_enabled", &person::notifications_enabled_)
      .add_column("notifications_slack_enabled", &person::notifications_slack_enabled_)
      .add_column("notifications_slack_userid", &person::notifications_slack_userid_)
      .add_column("notifications_mattermost_enabled", &person::notifications_mattermost_enabled_)
      .add_column("notifications_mattermost_userid", &person::notifications_mattermost_userid_)
      .add_column("notifications_discord_enabled", &person::notifications_discord_enabled_)
      .add_column("notifications_discord_userid", &person::notifications_discord_userid_)
      .add_column("is_bot", &person::is_bot_)
      .add_column("jti", &person::jti_, unique())
      .add_column("expiration_date", &person::expiration_date_)
      .add_column("studio_id", &person::studio_id_)
      .add_column("is_generated_from_ldap", &person::is_generated_from_ldap_)
      .add_column("ldap_uid", &person::ldap_uid_, unique())
      .add_column("dingding_id", &person::dingding_id_)
      .add_foreign_key("studio_id", &person::studio_id_, &studio::uuid_id_, foreign_key_action::set_null);

  reg_table<preview_background_file>("preview_background_file")
      .add_column("id", &preview_background_file::id_, primary_key(), autoincrement())
      .add_column("uuid", &preview_background_file::uuid_id_, not_null(), unique())
      .add_column("name", &preview_background_file::name_, not_null())
      .add_column("archived", &preview_background_file::archived_)
      .add_column("is_default", &preview_background_file::is_default_)
      .add_column("original_name", &preview_background_file::original_name_)
      .add_column("extension", &preview_background_file::extension_)
      .add_column("file_size", &preview_background_file::file_size_)
      .add_index("preview_background_file_is_default_index", &preview_background_file::is_default_);

  reg_table<status_automation>("status_automation")
      .add_column("id", &status_automation::id_, primary_key(), autoincrement())
      .add_column("uuid", &status_automation::uuid_id_, not_null(), unique())
      .add_column("entity_type", &status_automation::entity_type_)
      .add_column("in_task_type_id", &status_automation::in_task_type_id_)
      .add_column("in_task_status_id", &status_automation::in_task_status_id_)
      .add_column("out_field_type", &status_automation::out_field_type_)
      .add_column("out_task_type_id", &status_automation::out_task_type_id_)
      .add_column("out_task_status_id", &status_automation::out_task_status_id_)
      .add_column("import_last_revision", &status_automation::import_last_revision_)
      .add_column("archived", &status_automation::archived_)
      .add_foreign_key(
          "in_task_type_id", &status_automation::in_task_type_id_, &task_type::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "in_task_status_id", &status_automation::in_task_status_id_, &task_status::uuid_id_,
          foreign_key_action::cascade
      )
      .add_foreign_key(
          "out_task_type_id", &status_automation::out_task_type_id_, &task_type::uuid_id_, foreign_key_action::cascade
      )
      .add_foreign_key(
          "out_task_status_id", &status_automation::out_task_status_id_, &task_status::uuid_id_,
          foreign_key_action::cascade
      );

  reg_table<task_type>("task_type")
      .add_column("id", &task_type::id_, primary_key(), autoincrement())
      .add_column("uuid", &task_type::uuid_id_, not_null(), unique())
      .add_column("name", &task_type::name_, not_null())
      .add_column("short_name", &task_type::short_name_)
      .add_column("description", &task_type::description_)
      .add_column("color", &task_type::color_)
      .add_column("priority", &task_type::priority_)
      .add_column("for_entity", &task_type::for_entity_)
      .add_column("allow_timelog", &task_type::allow_timelog_)
      .add_column("archived", &task_type::archived_)
      .add_column("shotgun_id", &task_type::shotgun_id_)
      .add_column("department_id", &task_type::department_id_)
      .add_foreign_key("department_id", &task_type::department_id_, &department::uuid_id_, foreign_key_action::set_null)
      .add_unique_index("task_type_uc", &task_type::name_, &task_type::for_entity_, &task_type::department_id_);

  reg_table<department>("department")
      .add_column("id", &department::id_, primary_key(), autoincrement())
      .add_column("uuid", &department::uuid_id_, not_null(), unique())
      .add_column("name", &department::name_, not_null())
      .add_column("color", &department::color_, not_null())
      .add_column("archived", &department::archived_);

  reg_table<task_status>("task_status")
      .add_column("id", &task_status::id_, primary_key(), autoincrement())
      .add_column("uuid", &task_status::uuid_id_, not_null(), unique())
      .add_column("name", &task_status::name_, not_null())
      .add_column("archived", &task_status::archived_)
      .add_column("short_name", &task_status::short_name_, not_null())
      .add_column("description", &task_status::description_)
      .add_column("color", &task_status::color_, not_null())
      .add_column("priority", &task_status::priority_)
      .add_column("is_done", &task_status::is_done_)
      .add_column("is_artist_allowed", &task_status::is_artist_allowed_)
      .add_column("is_client_allowed", &task_status::is_client_allowed_)
      .add_column("is_retake", &task_status::is_retake_)
      .add_column("is_feedback_request", &task_status::is_feedback_request_)
      .add_column("is_default", &task_status::is_default_)
      .add_column("shotgun_id", &task_status::shotgun_id_)
      .add_column("for_concept", &task_status::for_concept_)
      .add_index("task_status_name_index", &task_status::name_)
      .add_index("task_status_short_name_index", &task_status::short_name_)
      .add_index("task_status_is_done_index", &task_status::is_done_)
      .add_index("task_status_is_default_index", &task_status::is_default_)
      .add_index("task_status_feedback_request_index", &task_status::is_feedback_request_);

  reg_table<asset_type>("asset_type")
      .add_column("id", &asset_type::id_, primary_key(), autoincrement())
      .add_column("uuid", &asset_type::uuid_id_, not_null(), unique())
      .add_column("name", &asset_type::name_, not_null(), unique())
      .add_column("short_name", &asset_type::short_name_)
      .add_column("description", &asset_type::description_)
      .add_column("archived", &asset_type::archived_);

  reg_table<studio>("studio")
      .add_column("id", &studio::id_, primary_key(), autoincrement())
      .add_column("uuid", &studio::uuid_id_, not_null(), unique())
      .add_column("name", &studio::name_, not_null(), unique())
      .add_column("color", &studio::color_, not_null())
      .add_column("app_key", &studio::app_key_)
      .add_column("app_secret", &studio::app_secret_)
      .add_column("archived", &studio::archived_);

  reg_table<organisation>("organisation")
      .add_column("id", &organisation::id_, primary_key(), autoincrement())
      .add_column("uuid", &organisation::uuid_id_, not_null(), unique())
      .add_column("name", &organisation::name_, not_null())
      .add_column("hours_by_day", &organisation::hours_by_day_, not_null())
      .add_column("has_avatar", &organisation::has_avatar_)
      .add_column("use_original_file_name", &organisation::use_original_file_name_)
      .add_column("timesheets_locked", &organisation::timesheets_locked_)
      .add_column("format_duration_in_hours", &organisation::format_duration_in_hours_)
      .add_column("hd_by_default", &organisation::hd_by_default_)
      .add_column("chat_token_slack", &organisation::chat_token_slack_)
      .add_column("chat_webhook_mattermost", &organisation::chat_webhook_mattermost_)
      .add_column("chat_token_discord", &organisation::chat_token_discord_)
      .add_column("dark_theme_by_default", &organisation::dark_theme_by_default_)
      .add_index("organisation_tab_uuid_id_index", &organisation::uuid_id_);
}

void sqlite_database::load(const FSys::path& in_path) {
  // auto l_list = {details::upgrade_init(in_path), details::upgrade_1(in_path)};
  // tokenizer::register_jieba_tokenizer(get_fts5_api());
  // for (auto&& i : l_list) {
  //   i->upgrade(impl_);
  // }
  open(in_path);
  pragma().foreign_keys(true);
  pragma().synchronous(1);
  pragma().recursive_triggers(true);
  pragma().journal_mode(orm::journal_mode_t::wal);
  // pragma().optimize(0x10002);
}
boost::asio::awaitable<void> sqlite_database::backup(FSys::path in_path) {
  DOODLE_TO_SQLITE_THREAD_2()
  // sqlite3* db_handle = static_cast<sqlite3*>(impl_->raw_sqlite_handle_);
  // if (db_handle) sqlite3_wal_checkpoint_v2(db_handle, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
  // if (db_handle) sqlite3_exec(db_handle, "PRAGMA optimize;", nullptr, nullptr, nullptr);
  backup_to(in_path);
  // impl_->storage_any_.vacuum();
  DOODLE_TO_SELF();
}

std::vector<attendance_helper::database_t> sqlite_database::get_attendance(
    const uuid& in_person_id, const chrono::local_days& in_data
) {
  using namespace orm;
  auto l_select = select(*this).columns(object<attendance_helper::database_t>());
  l_select.from<attendance_helper::database_t>().where(
      c(&attendance_helper::database_t::person_id_) == in_person_id &&
      c(&attendance_helper::database_t::create_date_) == in_data
  );
  return l_select().to_vector();
}
std::vector<attendance_helper::database_t> sqlite_database::get_attendance(
    const uuid& in_person_id, const std::vector<chrono::local_days>& in_data
) {
  using namespace orm;
  auto l_select = select(*this).columns(object<attendance_helper::database_t>());
  l_select.from<attendance_helper::database_t>().where(
      c(&attendance_helper::database_t::person_id_) == in_person_id &&
      c(&attendance_helper::database_t::create_date_).in(in_data)
  );
  return l_select().to_vector();
}
std::vector<work_xlsx_task_info_helper::database_t> sqlite_database::get_work_xlsx_task_info(
    const uuid& in_person_id, const chrono::local_days& in_data
) {
  using namespace orm;
  auto l_select = select(*this).columns(object<work_xlsx_task_info_helper::database_t>());
  l_select.from<work_xlsx_task_info_helper::database_t>().where(
      c(&work_xlsx_task_info_helper::database_t::person_id_) == in_person_id &&
      c(&work_xlsx_task_info_helper::database_t::year_month_) == in_data
  );
  return l_select().to_vector();
}

person sqlite_database::get_person_for_email(const std::string& in_email) {
  using namespace orm;
  auto l_p = select(get_sqlite_database())
                 .columns(object<person>())
                 .from<person>()
                 .where(c(&person::email_) == in_email)()
                 .to_single();
  return l_p;
}
std::vector<uuid> sqlite_database::get_temporal_type_ids() {
  using namespace orm;
  auto l_select = select(*this)
                      .columns(&asset_type::uuid_id_)
                      .from<asset_type>()
                      .where(c(&asset_type::name_).in({"Episode", "Sequence", "Shot", "Edit", "Scene", "Concept"}));
  return l_select().to_vector();
}

std::vector<project> sqlite_database::get_person_projects(const person& in_user) {
  using namespace orm;
  auto l_select = select(*this).columns(object<project>()).from<project>();
  l_select.join<project_status>(&project::project_status_id_, &project_status::uuid_id_)
      .join<project_person_link>(&project::uuid_id_, &project_person_link::project_id_)
      .where(
          c(&project_person_link::person_id_) == in_user.uuid_id_ &&
          c(&project_status::name_).in({"Active", "open", "Open"})
      );
  return l_select().to_vector();
}

std::optional<project_task_type_link> sqlite_database::get_project_task_type_link(
    const uuid& in_project_id, const uuid& in_task_type_id
) {
  // Migrated from sqlite_orm to doodle::orm
  using namespace orm;
  return select(*this)
      .columns(object<project_task_type_link>())
      .from<project_task_type_link>()
      .where(c(&project_task_type_link::project_id_) == in_project_id && c(&project_task_type_link::task_type_id_) == in_task_type_id)()
      .to_optional();
}

std::optional<project_task_status_link> sqlite_database::get_project_task_status_link(
    const uuid& in_project_id, const uuid& in_task_status_uuid
) {
  using namespace orm;
  return select(*this)
      .columns(object<project_task_status_link>())
      .from<project_task_status_link>()
      .where(c(&project_task_status_link::project_id_) == in_project_id && c(&project_task_status_link::task_status_id_) == in_task_status_uuid)()
      .to_optional();
}

std::optional<project_asset_type_link> sqlite_database::get_project_asset_type_link(
    const uuid& in_project_id, const uuid& in_asset_type_uuid
) {
  using namespace orm;
  return select(*this)
      .columns(object<project_asset_type_link>())
      .from<project_asset_type_link>()
      .where(c(&project_asset_type_link::project_id_) == in_project_id && c(&project_asset_type_link::asset_type_id_) == in_asset_type_uuid)()
      .to_optional();
}
bool sqlite_database::is_person_in_project(const person& in_person, const uuid& in_project_id) {
  return is_person_in_project(in_person.uuid_id_, in_project_id);
}
bool sqlite_database::is_person_in_project(const uuid& in_person, const uuid& in_project_id) {
  using namespace orm;
  return select(*this)
             .columns(count(&project_person_link::id_))
             .from<project_person_link>()
             .where(c(&project_person_link::project_id_) == in_project_id && c(&project_person_link::person_id_) == in_person)()
             .to_single() > 0;
}

bool sqlite_database::is_task_exist(const uuid& in_entity_id, const uuid& in_task_type_id) {
  using namespace orm;
  return select(*this)
             .columns(count(&task::id_))
             .from<task>()
             .where(c(&task::entity_id_) == in_entity_id && c(&task::task_type_id_) == in_task_type_id)()
             .to_single() > 0;
}
task_status sqlite_database::get_task_status_by_name(const std::string& in_name) {
  using namespace orm;
  return select(*this)
      .columns(object<task_status>())
      .from<task_status>()
      .where(c(&task_status::name_) == in_name)()
      .to_single();
}

std::set<uuid> sqlite_database::get_person_subscriptions(
    const person& in_person, const uuid& in_project_id, const uuid& in_asset_type_uuid
) {
  using namespace orm;

  dynamic_column_operations l_dynamic_column_operations{};
  l_dynamic_column_operations.add_condition(c(&subscription::person_id_) == in_person.uuid_id_);
  if (!in_project_id.is_nil()) l_dynamic_column_operations.add_condition(c(&task::project_id_) == in_project_id);
  if (!in_asset_type_uuid.is_nil())
    l_dynamic_column_operations.add_condition(c(&entity::entity_type_id_) == in_asset_type_uuid);
  else
    l_dynamic_column_operations.add_condition(c(&entity::entity_type_id_).in(get_temporal_type_ids()));

  return select(*this)
      .columns(&subscription::task_id_)
      .from<subscription>()
      .join<task>(&subscription::task_id_, &task::uuid_id_)
      .join<entity>(&task::entity_id_, &entity::uuid_id_)
      .where(l_dynamic_column_operations)()
      .to_set();
}

asset_type sqlite_database::get_entity_type_by_name(const std::string& in_name) {
  using namespace sqlite_orm;
  auto l_e = impl_->storage_any_.get_all<asset_type>(where(c(&asset_type::name_) == in_name));
  if (l_e.empty()) throw_exception(doodle_error{"未知的实体类型 {}", in_name});
  return l_e.front();
}

std::vector<person> sqlite_database::get_project_persons(const uuid& in_project_uuid) {
  using namespace sqlite_orm;
  return impl_->storage_any_.get_all<person>(
      join<project_person_link>(on(c(&project_person_link::person_id_) == c(&person::uuid_id_))),
      where(c(&project_person_link::project_id_) == in_project_uuid)
  );
}

std::set<uuid> sqlite_database::get_notification_recipients(const task& in_task) {
  using namespace sqlite_orm;

  auto l_uuid_task =
      impl_->storage_any_.select(&subscription::uuid_id_, where(c(&subscription::task_id_) == in_task.uuid_id_));
  if (impl_->uuid_to_id<entity>(in_task.uuid_id_))
    if (auto l_entt = impl_->get_by_uuid<entity>(in_task.entity_id_); !l_entt.project_id_.is_nil())
      l_uuid_task |= ranges::actions::push_back(impl_->storage_any_.select(
          &subscription::uuid_id_, where(
                                       c(&subscription::task_type_id_) == in_task.task_type_id_ &&
                                       c(&subscription::entity_id_) == l_entt.project_id_
                                   )
      ));
  l_uuid_task |= ranges::actions::push_back(
      impl_->storage_any_.select(&assignees_table::person_id_, where(c(&assignees_table::task_id_) == in_task.uuid_id_))
  );

  return l_uuid_task | ranges::to<std::set<uuid>>();
}
std::set<uuid> sqlite_database::get_mentioned_people(const uuid& in_project_id, const comment& in_comment_id) {
  using namespace sqlite_orm;

  auto l_mentions = in_comment_id.mentions_;
  for (auto&& i : in_comment_id.department_mentions_)
    l_mentions |= ranges::actions::push_back(impl_->storage_any_.select(
        &person::uuid_id_,
        join<person_department_link>(on(c(&person_department_link::person_id_) == c(&person::uuid_id_))),
        join<project_person_link>(on(c(&project_person_link::person_id_) == c(&person::uuid_id_))),
        where(c(&project_person_link::project_id_) == in_project_id && c(&person_department_link::department_id_) == i)

    ));
  return l_mentions | ranges::to<std::set<uuid>>();
}

std::vector<status_automation> sqlite_database::get_project_status_automations(const uuid& in_project_uuid) {
  using namespace sqlite_orm;
  return impl_->storage_any_.get_all<status_automation>(
      join<project_status_automation_link>(
          on(c(&project_status_automation_link::status_automation_id_) == c(&status_automation::uuid_id_))
      ),
      where(c(&project_status_automation_link::project_id_) == in_project_uuid)
  );
}

std::map<uuid, std::int32_t> sqlite_database::get_task_type_priority_map(
    const uuid& in_project, const std::string& in_for_entity
) {
  using namespace sqlite_orm;

  auto l_t = impl_->storage_any_.select(
      columns(&project_task_type_link::uuid_id_, &project_task_type_link::priority_),
      join<task_type>(on(c(&project_task_type_link::task_type_id_) == c(&task_type::uuid_id_))),
      where(c(&project_task_type_link::project_id_) == in_project && c(&task_type::for_entity_) == in_for_entity)
  );

  std::map<uuid, std::int32_t> l_ret{};
  for (auto&& [key, value] : l_t) {
    l_ret[key] = value.value_or(0);
  }
  return l_ret;
}

std::optional<task> sqlite_database::get_tasks_for_entity_and_task_type(
    const uuid& in_entity_id, const uuid& in_task_type_id
) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<task>(
      where(c(&task::entity_id_) == in_entity_id && c(&task::task_type_id_) == in_task_type_id)
  );
  return l_t.empty() ? std::nullopt : std::make_optional(l_t.front());
}

bool sqlite_database::has_assets_tree_assets_link(const uuid& in_uuid) {
  using namespace sqlite_orm;

  return impl_->storage_any_.count<assets_file_helper::link_parent_t>(
             where(c(&assets_file_helper::link_parent_t::assets_type_uuid_) == in_uuid)
         ) > 0;
}
bool sqlite_database::has_assets_tree_assets_link(const uuid& in_label_uuid, const uuid& in_asset_uuid) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.count<assets_file_helper::link_parent_t>(where(
      c(&assets_file_helper::link_parent_t::assets_type_uuid_) == in_label_uuid &&
      c(&assets_file_helper::link_parent_t::assets_uuid_) == in_asset_uuid
  ));
  return l_t > 0;
}
assets_file_helper::link_parent_t sqlite_database::get_assets_tree_assets_link(
    const uuid& in_label_uuid, const uuid& in_asset_uuid
) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<assets_file_helper::link_parent_t>(where(
      c(&assets_file_helper::link_parent_t::assets_type_uuid_) == in_label_uuid &&
      c(&assets_file_helper::link_parent_t::assets_uuid_) == in_asset_uuid
  ));
  if (l_t.empty()) throw_exception(doodle_error{"未知的标签资产链接 {} {}", in_label_uuid, in_asset_uuid});
  return l_t.front();
}
bool sqlite_database::has_assets_tree_child(const uuid& in_label_uuid) {
  using namespace sqlite_orm;
  auto l_r = impl_->storage_any_.count<assets_helper::database_t>(
      where(c(&assets_helper::database_t::uuid_parent_) == in_label_uuid)
  );
  return l_r > 0;
}
auto mix_preview_file_revisions(const std::vector<preview_files_for_entity_t>& in_t) {
  std::map<std::int32_t, preview_files_for_entity_t> l_map{};
  std::vector<preview_files_for_entity_t> l_ret{};
  for (auto&& i : in_t) {
    if (!l_map.contains(i.revision_)) {
      l_map[i.revision_] = i;
      l_ret.emplace_back(i);
    } else {
      auto& l_task = l_map[i.revision_];
      l_task.previews_.emplace_back(i);
    }
  }
  return l_ret;
}
std::map<uuid, std::vector<preview_files_for_entity_t>> sqlite_database::get_preview_files_for_entity(
    const uuid& in_entity_id
) {
  using namespace sqlite_orm;
  std::map<uuid, std::vector<preview_files_for_entity_t>> l_ret{};

  auto l_t = impl_->storage_any_.select(
      columns(
          &task::uuid_id_, &task_type::uuid_id_, &preview_file::uuid_id_, &preview_file::revision_,
          &preview_file::position_, &preview_file::original_name_, &preview_file::extension_, &preview_file::width_,
          &preview_file::height_, &preview_file::duration_, &preview_file::status_, &preview_file::source_,
          &preview_file::annotations_, &preview_file::created_at_
      ),
      // from<task>(),
      join<preview_file>(on(c(&preview_file::task_id_) == c(&task::uuid_id_))),
      join<task_type>(on(c(&task::task_type_id_) == c(&task_type::uuid_id_))),
      where(c(&task::entity_id_) == in_entity_id),
      multi_order_by(
          order_by(&task_type::priority_).desc(), order_by(&task_type::name_),
          order_by(&preview_file::revision_).desc(), order_by(&preview_file::created_at_)
      )
  );
  std::map<uuid, std::vector<preview_files_for_entity_t>> l_select{};
  for (auto&& [task_id, task_type_id, preview_id, revision, position, original_name, extension, width, height, duration, status, source_, annotations, created_at] :
       l_t) {
    l_select[task_id].emplace_back(
        preview_files_for_entity_t{
            .uuid_id_       = preview_id,
            .revision_      = revision,
            .position_      = position,
            .original_name_ = original_name,
            .extension_     = extension,
            .width_         = width,
            .height_        = height,
            .duration_      = duration,
            .status_        = status,
            .source_        = source_,
            .annotations_   = annotations,
            .created_at_    = created_at,
            .task_id_       = task_id,
            .task_type_id_  = task_type_id
        }
    );
  }

  for (auto&& keys : l_select | ranges::views::keys) {
    auto l_preview_files = l_select[keys];
    if (l_preview_files.empty()) continue;
    auto l_task_type_id   = l_preview_files.front().task_type_id_;
    auto l_pres           = mix_preview_file_revisions(l_preview_files);
    l_ret[l_task_type_id] = l_pres;
  }
  return l_ret;
}
std::optional<preview_file> sqlite_database::get_preview_file_for_comment(const uuid& in_comment_id) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<comment_preview_link>(
      where(c(&comment_preview_link::comment_id_) == in_comment_id), limit(1)
  );
  if (l_t.empty()) return std::nullopt;
  return impl_->get_by_uuid<preview_file>(l_t.front().preview_file_id_);
}

bool sqlite_database::is_task_assigned_to_person(const uuid& in_task, const uuid& in_person) {
  using namespace sqlite_orm;
  auto l_r = impl_->storage_any_.count<assignees_table>(
      where(c(&assignees_table::task_id_) == in_task && c(&assignees_table::person_id_) == in_person)
  );
  return l_r > 0;
}
std::int64_t sqlite_database::get_next_preview_revision(const uuid& in_task_id) {
  using namespace sqlite_orm;
  auto l_values = impl_->storage_any_.select(
      &preview_file::revision_, where(c(&preview_file::task_id_) == in_task_id),
      order_by(&preview_file::revision_).desc()
  );
  return l_values.empty() ? 1 : l_values.front() + 1;
}
bool sqlite_database::has_preview_file(const uuid& in_comment) {
  using namespace sqlite_orm;
  auto l_r =
      impl_->storage_any_.count<comment_preview_link>(where(c(&comment_preview_link::comment_id_) == in_comment));
  return l_r > 0;
}
std::int64_t sqlite_database::get_next_position(const uuid& in_task_id, const std::int64_t& in_revision) {
  using namespace sqlite_orm;

  auto l_r = impl_->storage_any_.count<preview_file>(
      where(c(&preview_file::task_id_) == in_task_id && c(&preview_file::revision_) == in_revision)
  );
  return l_r + 1;
}
std::int64_t sqlite_database::get_preview_revision(const uuid& in_comment) {
  using namespace sqlite_orm;

  auto l_r = impl_->storage_any_.get_all<comment_preview_link>(
      where(c(&comment_preview_link::comment_id_) == in_comment), limit(1)
  );
  if (l_r.empty()) return 0;
  return impl_->get_by_uuid<preview_file>(l_r.front().preview_file_id_).revision_;
}

std::optional<comment> sqlite_database::get_last_comment(const uuid& in_task_id) {
  using namespace sqlite_orm;
  auto l_r = impl_->storage_any_.get_all<comment>(
      where(c(&comment::object_id_) == in_task_id), order_by(&comment::created_at_).desc(), limit(1)
  );
  if (l_r.empty()) return std::nullopt;
  return l_r.front();
}
std::vector<task> sqlite_database::get_tasks_for_entity(const uuid& in_asset_id) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<task>(where(c(&task::entity_id_) == in_asset_id));
  return l_t;
}

std::vector<asset_type> sqlite_database::get_asset_types_not_temporal_type() {
  using namespace sqlite_orm;
  auto l_te            = get_temporal_type_ids();
  auto l_t             = impl_->storage_any_.get_all<asset_type>(where(not_in(&asset_type::uuid_id_, l_te)));

  auto l_ass_type_link = impl_->storage_any_.get_all<task_type_asset_type_link>(
      where(not_in(&task_type_asset_type_link::asset_type_id_, l_te))
  );
  std::map<uuid, std::vector<uuid>> l_ass_type_link_map{};
  for (auto& i : l_ass_type_link) l_ass_type_link_map[i.asset_type_id_].push_back(i.task_type_id_);
  for (auto& i : l_t) {
    i.task_types_ = l_ass_type_link_map[i.uuid_id_];
  }
  return l_t;
}

std::optional<entity_link> sqlite_database::get_entity_link(const uuid& in_entity_in_id, const uuid& in_asset_id) {
  using namespace sqlite_orm;
  auto l_list = impl_->storage_any_.get_all<entity_link>(
      where(c(&entity_link::entity_in_id_) == in_entity_in_id && c(&entity_link::entity_out_id_) == in_asset_id)
  );
  if (l_list.empty()) return {};
  return l_list.front();
}

boost::asio::awaitable<void> sqlite_database::mark_all_notifications_as_read(uuid in_user_id) {
  DOODLE_TO_SQLITE_THREAD_2();
  using namespace sqlite_orm;

  auto l_g = impl_->storage_any_.transaction_guard();
  impl_->storage_any_.update_all(
      set(c(&notification::read_) = true),
      where(c(&notification::person_id_) == in_user_id && c(&notification::read_) == false)
  );
  l_g.commit();
  DOODLE_TO_SELF();
  co_return;
}

std::optional<entity_asset_extend> sqlite_database::get_entity_asset_extend(const uuid& in_entity_id) {
  using namespace sqlite_orm;
  auto l_t =
      impl_->storage_any_.get_all<entity_asset_extend>(where(c(&entity_asset_extend::entity_id_) == in_entity_id));
  if (l_t.empty()) return std::nullopt;
  return l_t.front();
}

std::optional<entity_shot_extend> sqlite_database::get_entity_shot_extend(const uuid& in_entity_id) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<entity_shot_extend>(where(c(&entity_shot_extend::entity_id_) == in_entity_id));
  if (l_t.empty()) return std::nullopt;
  return l_t.front();
}

std::vector<playlist_shot> sqlite_database::get_playlist_shot_entity(const uuid& in_playlist_id) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<playlist_shot>(
      where(c(&playlist_shot::playlist_id_) == in_playlist_id), order_by(&playlist_shot::order_index_)
  );
  return l_t;
}
boost::asio::awaitable<void> sqlite_database::remove_playlist_shot_for_playlist(const uuid& in_playlist_id) {
  using namespace sqlite_orm;
  DOODLE_TO_SQLITE_THREAD_2();
  impl_->storage_any_.remove_all<playlist_shot>(where(c(&playlist_shot::playlist_id_) == in_playlist_id));
  DOODLE_TO_SELF();
  co_return;
}
std::optional<task_type_asset_type_link> sqlite_database::get_task_type_asset_type_link(
    const uuid& in_task_type_id, const uuid& in_asset_type_id
) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<task_type_asset_type_link>(where(
      c(&task_type_asset_type_link::task_type_id_) == in_task_type_id &&
      c(&task_type_asset_type_link::asset_type_id_) == in_asset_type_id
  ));
  if (l_t.empty()) return std::nullopt;
  return l_t.front();
}
boost::asio::awaitable<void> sqlite_database::remove_task_type_asset_type_link_by_asset_type(
    const uuid& in_asset_type_id
) {
  using namespace sqlite_orm;
  DOODLE_TO_SQLITE_THREAD_2();
  impl_->storage_any_.remove_all<task_type_asset_type_link>(
      where(c(&task_type_asset_type_link::asset_type_id_) == in_asset_type_id)
  );
  DOODLE_TO_SELF();
  co_return;
}

uuid sqlite_database::get_project_status_open() const {
  using namespace sqlite_orm;
  auto l_list = impl_->storage_any_.get_all<project_status>(where(c(&project_status::name_) == "Open"));
  if (l_list.empty()) throw_exception(doodle_error{"Open状态不存在"});
  return l_list.front().uuid_id_;
}

uuid sqlite_database::get_project_status_closed() const {
  using namespace sqlite_orm;
  auto l_list = impl_->storage_any_.get_all<project_status>(where(c(&project_status::name_) == "Closed"));
  if (l_list.empty()) throw_exception(doodle_error{"Closed状态不存在"});
  return l_list.front().uuid_id_;
}

std::size_t sqlite_database::get_project_entity_count(const uuid& in_project_id) {
  using namespace sqlite_orm;
  return impl_->storage_any_.count<entity>(where(in_project_id.is_nil() || c(&entity::project_id_) == in_project_id));
}

bool sqlite_database::is_entity_outsourced(
    const uuid& in_entity_id, const uuid& in_studio_id, const uuid& in_parent_id
) {
  using namespace sqlite_orm;
  std::vector<uuid> l_entity_ids{};
  l_entity_ids.push_back(in_entity_id);
  if (!in_parent_id.is_nil()) l_entity_ids.push_back(in_parent_id);
  auto l_r = impl_->storage_any_.count<outsource_studio_authorization>(where(
      c(&outsource_studio_authorization::studio_id_) == in_studio_id &&
      in(&outsource_studio_authorization::entity_id_, l_entity_ids)
  ));
  return l_r > 0;
}

boost::asio::awaitable<void> sqlite_database::remove_sequence_casting(const uuid& in_sequence_id) {
  DOODLE_TO_SQLITE_THREAD_2();
  using namespace sqlite_orm;
  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();
  impl_->storage_any_.remove_all<entity_link>(where(
      in(&entity_link::id_,
         select(
             &entity_link::id_, join<shot>(on(c(&entity_link::entity_in_id_) == c(shot->*&entity::uuid_id_))),
             join<sequence>(on(c(shot->*&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
             where(c(sequence->*&entity::uuid_id_) == in_sequence_id)
         ))
  ));
  DOODLE_TO_SELF();
  co_return;
}

std::vector<server_task_info> sqlite_database::get_server_tasks_by_submitted() {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<server_task_info>(
      where(c(&server_task_info::status_) == server_task_info_status::submitted),
      multi_order_by(order_by(&server_task_info::priority_).desc(), order_by(&server_task_info::submit_time_))
  );
  return l_t;
}

entity_asset_extend sqlite_database::get_entity_shot_extend_by_task(const uuid& in_shot_id) {
  using namespace sqlite_orm;
  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();
  auto l_assets           = impl_->storage_any_.select(
      columns(object<entity>(true), object<entity_asset_extend>(true)), from<entity>(),
      left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
      where(
          in(&entity::uuid_id_,
                       select(
                 &entity_link::entity_out_id_, from<entity_link>(),
                 join<shot>(on(c(&entity_link::entity_in_id_) == c(shot->*&entity::uuid_id_))),
                 join<sequence>(on(c(shot->*&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
                 where(c(shot->*&entity::uuid_id_) == in_shot_id)
             )) &&
          !c(&entity::canceled_) && c(&entity::entity_type_id_) == asset_type::get_ground_id()
      )
  );
  DOODLE_CHICK(l_assets.size() == 1, "错误, 找个了 {} 个对应的地编资产", l_assets.size());
  auto& [shot_entity, shot_extend] = l_assets.front();
  return {shot_extend};
}
boost::asio::awaitable<void> sqlite_database::update_computer_status(
    const uuid& in_computer_id, computer_status in_status
) {
  DOODLE_TO_SQLITE_THREAD_2();
  using namespace sqlite_orm;
  impl_->storage_any_.update_all(
      set(c(&computer::status_) = in_status), where(c(&computer::uuid_id_) == in_computer_id)
  );
  DOODLE_TO_SELF();
}

bool sqlite_database::is_person_ai_studio_connected(const uuid& in_person_id, const uuid& in_ai_studio_id) const {
  using namespace sqlite_orm;
  auto l_r = impl_->storage_any_.count<ai_studio_person_role_link>(where(
      c(&ai_studio_person_role_link::person_id_) == in_person_id &&
      c(&ai_studio_person_role_link::ai_studio_id_) == in_ai_studio_id
  ));
  return l_r > 0;
}
std::optional<ai_studio_person_role_link> sqlite_database::get_ai_studio_person_role_link(
    const uuid& in_person_id, const uuid& in_ai_studio_id
) const {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<ai_studio_person_role_link>(where(
      c(&ai_studio_person_role_link::person_id_) == in_person_id &&
      c(&ai_studio_person_role_link::ai_studio_id_) == in_ai_studio_id
  ));
  if (l_t.empty()) return std::nullopt;
  return l_t.front();
}

}  // namespace doodle