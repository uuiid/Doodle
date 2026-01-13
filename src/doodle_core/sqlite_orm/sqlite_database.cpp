//
// Created by TD on 24-9-12.
//

#include "sqlite_database.h"

#include "doodle_core_fwd.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/asset_instance.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>
#include <doodle_core/sqlite_orm/sqlite_upgrade.h>

#include "metadata/ai_image_metadata.h"
#include "metadata/entity.h"
#include "metadata/entity_type.h"
#include "metadata/playlist.h"
#include "metadata/preview_file.h"
#include "metadata/working_file.h"
#include <optional>
#include <sqlite_orm/sqlite_orm.h>

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
void sqlite_database_error_log_callback(void* pArg, int iErrCode, const char* zMsg) {
  if (auto l_logger = static_cast<spdlog::logger*>(pArg); l_logger)
    l_logger->error(fmt::format("{} {}", iErrCode, zMsg));
}
}  // namespace
void sqlite_database::load(const FSys::path& in_path) {
  static std::once_flag l_flag{};
  std::call_once(l_flag, [this]() {
    this->logger_ = std::make_shared<spdlog::async_logger>(
        "sqlite",
        spdlog::sinks_init_list{
            g_logger_ctrl().rotating_file_sink_
#ifndef NDEBUG
            ,
            g_logger_ctrl().debug_sink_
#endif
        },
        spdlog::thread_pool()
    );
    sqlite3_config(SQLITE_CONFIG_LOG, sqlite_database_error_log_callback, this->logger_.get());
  });

  auto l_list = {details::upgrade_init(in_path), details::upgrade_1(in_path)};
  impl_       = std::make_shared<sqlite_database_impl>(in_path);
  for (auto&& i : l_list) {
    i->upgrade(impl_);
  }
  impl_->storage_any_.pragma.synchronous(1);
  impl_->storage_any_.pragma.journal_mode(sqlite_orm::journal_mode::WAL);
}
boost::asio::awaitable<void> sqlite_database::backup(FSys::path in_path) {
  DOODLE_TO_SQLITE_THREAD_2()

  impl_->storage_any_.backup_to(in_path.generic_string());
  DOODLE_TO_SELF();
}

std::vector<attendance_helper::database_t> sqlite_database::get_attendance(
    const uuid& in_person_id, const chrono::local_days& in_data
) {
  using namespace sqlite_orm;
  return impl_->storage_any_.get_all<attendance_helper::database_t>(where(
      c(&attendance_helper::database_t::person_id_) == in_person_id &&
      c(&attendance_helper::database_t::create_date_) == in_data
  ));
}
std::vector<attendance_helper::database_t> sqlite_database::get_attendance(
    const uuid& in_person_id, const std::vector<chrono::local_days>& in_data
) {
  using namespace sqlite_orm;
  return impl_->storage_any_.get_all<attendance_helper::database_t>(where(
      c(&attendance_helper::database_t::person_id_) == in_person_id &&
      in(&attendance_helper::database_t::create_date_, in_data)
  ));
}
std::vector<work_xlsx_task_info_helper::database_t> sqlite_database::get_work_xlsx_task_info(
    const uuid& in_person_id, const chrono::local_days& in_data
) {
  using namespace sqlite_orm;
  return impl_->storage_any_.get_all<work_xlsx_task_info_helper::database_t>(where(
      c(&work_xlsx_task_info_helper::database_t::person_id_) == in_person_id &&
      c(&work_xlsx_task_info_helper::database_t::year_month_) == in_data
  ));
}

std::int32_t sqlite_database::get_notification_count(const uuid& in_user_id) {
  using namespace sqlite_orm;
  return impl_->storage_any_.count<notification>(where(c(&notification::person_id_) == in_user_id));
}

std::vector<project_with_extra_data> sqlite_database::get_project_for_user(const person& in_user) {
  std::vector<project_with_extra_data> l_projects{};
  if (in_user.role_ == person_role_type::admin) {
    auto l_t = impl_->storage_any_.get_all<project>(
        sqlite_orm::join<project_status>(
            sqlite_orm::on(sqlite_orm::c(&project_status::uuid_id_) == sqlite_orm::c(&project::project_status_id_))
        ),
        sqlite_orm::where(sqlite_orm::in(&project_status::name_, {"Active", "open", "Open"}))
    );
    l_projects = l_t | ranges::views::transform([](const project& in) { return project_with_extra_data{in}; }) |
                 ranges::to_vector;
  } else {
    auto l_t   = get_person_projects(in_user);
    l_projects = l_t | ranges::views::transform([](const project& in) { return project_with_extra_data{in}; }) |
                 ranges::to_vector;
  }
  auto l_descriptors = impl_->storage_any_.get_all<metadata_descriptor>();
  {
    auto l_departments = impl_->storage_any_.select(&department::uuid_id_);
    for (auto&& i : l_descriptors) {
      i.department_ = l_departments;
    }
  }
  for (auto&& i : l_projects) {
    auto l_project_person_link = impl_->storage_any_.get_all<project_person_link>(
        sqlite_orm::where(sqlite_orm::c(&project_person_link::project_id_) == i.uuid_id_)
    );
    i.team_ = l_project_person_link |
              ranges::views::transform([](const project_person_link& in) { return in.person_id_; }) | ranges::to_vector;
    auto l_asset_type_link = impl_->storage_any_.get_all<project_asset_type_link>(
        sqlite_orm::where(sqlite_orm::c(&project_asset_type_link::project_id_) == i.uuid_id_)
    );
    i.asset_types_ = l_asset_type_link |
                     ranges::views::transform([](const project_asset_type_link& in) { return in.asset_type_id_; }) |
                     ranges::to_vector;
    auto l_task_status_link = impl_->storage_any_.get_all<project_task_status_link>(
        sqlite_orm::where(sqlite_orm::c(&project_task_status_link::project_id_) == i.uuid_id_)
    );
    i.task_statuses_ = l_task_status_link |
                       ranges::views::transform([](const project_task_status_link& in) { return in.task_status_id_; }) |
                       ranges::to_vector;
    auto l_task_type_link = impl_->storage_any_.get_all<project_task_type_link>(
        sqlite_orm::where(sqlite_orm::c(&project_task_type_link::project_id_) == i.uuid_id_)
    );
    i.task_types_ = l_task_type_link |
                    ranges::views::transform([](const project_task_type_link& in) { return in.task_type_id_; }) |
                    ranges::to_vector;
    auto l_status_automations = impl_->storage_any_.get_all<project_status_automation_link>(
        sqlite_orm::where(sqlite_orm::c(&project_status_automation_link::project_id_) == i.uuid_id_)
    );
    i.status_automations_ =
        l_status_automations |
        ranges::views::transform([](const project_status_automation_link& in) { return in.status_automation_id_; }) |
        ranges::to_vector;
    auto l_preview_background_files = impl_->storage_any_.get_all<project_preview_background_file_link>(
        sqlite_orm::where(sqlite_orm::c(&project_preview_background_file_link::project_id_) == i.uuid_id_)
    );
    i.preview_background_files_ = l_preview_background_files |
                                  ranges::views::transform([](const project_preview_background_file_link& in) {
                                    return in.preview_background_file_id_;
                                  }) |
                                  ranges::to_vector;

    i.descriptors_ =
        l_descriptors |
        ranges::views::filter([&](const metadata_descriptor& in) { return in.project_uuid_ == i.uuid_id_; }) |
        ranges::to_vector;
    i.task_types_priority_ = l_task_type_link;
    i.task_statuses_link_  = l_task_status_link;
  }
  return l_projects;
}
person sqlite_database::get_person_for_email(const std::string& in_email) {
  auto l_p = impl_->storage_any_.get_all<person>(sqlite_orm::where(sqlite_orm::c(&person::email_) == in_email));
  if (l_p.empty()) throw_exception(doodle_error{"未知的用户"});
  return l_p.front();
}
std::vector<uuid> sqlite_database::get_temporal_type_ids() {
  return impl_->storage_any_.select(
      &asset_type::uuid_id_,
      sqlite_orm::where(sqlite_orm::in(&asset_type::name_, {"Episode", "Sequence", "Shot", "Edit", "Scene", "Concept"}))
  );
}

std::vector<project> sqlite_database::get_person_projects(const person& in_user) {
  std::vector<project> l_result;
  auto l_ids = impl_->storage_any_.get_all<project>(
      sqlite_orm::join<project_status>(
          sqlite_orm::on(sqlite_orm::c(&project_status::uuid_id_) == sqlite_orm::c(&project::project_status_id_))
      ),
      sqlite_orm::join<project_person_link>(
          sqlite_orm::on(sqlite_orm::c(&project_person_link::project_id_) == sqlite_orm::c(&project::uuid_id_))
      ),
      sqlite_orm::where(
          sqlite_orm::c(&project_person_link::person_id_) == in_user.uuid_id_ &&
          sqlite_orm::in(&project_status::name_, {"Active", "open", "Open"})
      )
  );
  l_result.reserve(l_ids.size());
  for (auto&& l_id : l_ids) l_result.emplace_back(l_id);
  return l_result;
}
std::vector<project_and_status_t> sqlite_database::get_project_and_status(const person& in_user) {
  using namespace sqlite_orm;
  static constexpr auto sql_orm_project_and_status_t = sqlite_orm::struct_<project_and_status_t>(
      &project::uuid_id_, &project::name_, &project::code_, &project::description_, &project::shotgun_id_,
      &project::file_tree_, &project::data_, &project::has_avatar_, &project::fps_, &project::ratio_,
      &project::resolution_, &project::production_type_, &project::production_style_, &project::start_date_,
      &project::end_date_, &project::man_days_, &project::nb_episodes_, &project::episode_span_, &project::max_retakes_,
      &project::is_clients_isolated_, &project::is_preview_download_allowed_, &project::is_set_preview_automated_,
      &project::homepage_, &project::is_publish_default_for_artists_, &project::hd_bitrate_compression_,
      &project::ld_bitrate_compression_, &project::project_status_id_, &project::default_preview_background_file_id_,
      &project_status::name_
  );
  auto l_r = impl_->storage_any_.select(
      sql_orm_project_and_status_t,
      join<project_status>(on(c(&project::project_status_id_) == c(&project_status::uuid_id_))),
      join<project_person_link>(on(c(&project_person_link::project_id_) == c(&project::uuid_id_))),
      where(in_user.uuid_id_.is_nil() || c(&project_person_link::person_id_) == in_user.uuid_id_)
  );
  return l_r;
}

std::vector<get_comments_t> sqlite_database::get_comments(const uuid& in_task_id) {
  using namespace sqlite_orm;
  auto l_comm = impl_->storage_any_.get_all<comment>(
      where(c(&comment::object_id_) == in_task_id), order_by(&comment::created_at_).desc()
  );

  auto l_person_ids =
      l_comm | ranges::views::transform([](const comment& in) { return in.person_id_; }) | ranges::to_vector;
  l_person_ids |= ranges::actions::remove_if([](const uuid& in) { return in.is_nil(); }) | ranges::actions::unique;
  auto l_task_status_ids =
      l_comm | ranges::views::transform([](const comment& in) { return in.task_status_id_; }) | ranges::to_vector;
  l_task_status_ids |= ranges::actions::remove_if([](const uuid& in) { return in.is_nil(); }) | ranges::actions::unique;
  auto l_editor_ids =
      l_comm | ranges::views::transform([](const comment& in) { return in.editor_id_; }) | ranges::to_vector;
  l_editor_ids |= ranges::actions::remove_if([](const uuid& in) { return in.is_nil(); }) | ranges::actions::unique;
  auto l_comment_ids =
      l_comm | ranges::views::transform([](const comment& in) { return in.uuid_id_; }) | ranges::to_vector;
  auto l_persons       = impl_->storage_any_.get_all<person>(where(in(&person::uuid_id_, l_person_ids)));
  auto l_task_statuses = impl_->storage_any_.get_all<task_status>(where(in(&task_status::uuid_id_, l_task_status_ids)));
  auto l_editors       = impl_->storage_any_.get_all<person>(where(in(&person::uuid_id_, l_editor_ids)));
  auto l_acknowledgements = impl_->storage_any_.get_all<comment_acknoledgments>(
      where(in(&comment_acknoledgments::comment_id_, l_comment_ids))
  );

  struct previews_and_comment_id_t {
    decltype(preview_file::uuid_id_) uuid_id_;
    decltype(preview_file::task_id_) task_id_;
    decltype(preview_file::revision_) revision_;
    decltype(preview_file::extension_) extension_;
    decltype(preview_file::width_) width_;
    decltype(preview_file::height_) height_;
    decltype(preview_file::duration_) duration_;
    decltype(preview_file::status_) status_;
    decltype(preview_file::validation_status_) validation_status_;
    decltype(preview_file::original_name_) original_name_;
    decltype(preview_file::position_) position_;
    decltype(preview_file::annotations_) annotations_;

    decltype(comment_preview_link::comment_id_) comment_id_;
  };
  constexpr auto sql_orm_previews_and_comment_id_t = struct_<previews_and_comment_id_t>(
      &preview_file::uuid_id_, &preview_file::task_id_, &preview_file::revision_, &preview_file::extension_,
      &preview_file::width_, &preview_file::height_, &preview_file::duration_, &preview_file::status_,
      &preview_file::validation_status_, &preview_file::original_name_, &preview_file::position_,
      &preview_file::annotations_, &comment_preview_link::comment_id_
  );
  auto l_previews = impl_->storage_any_.select(
      sql_orm_previews_and_comment_id_t,
      join<comment_preview_link>(on(c(&comment_preview_link::preview_file_id_) == c(&preview_file::uuid_id_))),
      where(in(&comment_preview_link::comment_id_, l_comment_ids))
  );
  auto l_mention =
      impl_->storage_any_.get_all<comment_mentions>(where(in(&comment_mentions::comment_id_, l_comment_ids)));
  auto l_department_mention = impl_->storage_any_.get_all<comment_department_mentions>(
      where(in(&comment_department_mentions::comment_id_, l_comment_ids))
  );
  auto l_attachment_file =
      impl_->storage_any_.get_all<attachment_file>(where(in(&attachment_file::comment_id_, l_comment_ids)));

  auto l_map_person = l_persons |
                      ranges::views::transform([](const person& in) { return std::make_pair(in.uuid_id_, &in); }) |
                      ranges::to<std::map<uuid, const person*>>();

  auto l_map_task_status =
      l_task_statuses |
      ranges::views::transform([](const task_status& in) { return std::make_pair(in.uuid_id_, &in); }) |
      ranges::to<std::map<uuid, const task_status*>>();

  auto l_map_editor = l_editors |
                      ranges::views::transform([](const person& in) { return std::make_pair(in.uuid_id_, &in); }) |
                      ranges::to<std::map<uuid, const person*>>();

  std::map<uuid, std::vector<uuid>> l_map_acknowledgements;
  for (auto&& l_a : l_acknowledgements) l_map_acknowledgements[l_a.comment_id_].push_back(l_a.person_id_);

  std::map<uuid, std::vector<previews_and_comment_id_t*>> l_map_previews{};
  for (auto&& l_p : l_previews) l_map_previews[l_p.comment_id_].push_back(&l_p);

  std::map<uuid, std::vector<uuid>> l_map_mentions{};
  for (auto&& l_m : l_mention) l_map_mentions[l_m.comment_id_].push_back(l_m.person_id_);

  std::map<uuid, std::vector<uuid>> l_map_department_mentions{};
  for (auto&& l_m : l_department_mention) l_map_department_mentions[l_m.comment_id_].push_back(l_m.department_id_);

  std::map<uuid, std::vector<attachment_file*>> l_map_attachment_file{};
  for (auto&& l_m : l_attachment_file) l_map_attachment_file[l_m.comment_id_].push_back(&l_m);

  std::vector<get_comments_t> l_result;
  l_result.reserve(l_comm.size());
  for (auto&& l_c : l_comm) {
    get_comments_t l_comment{
        .uuid_id_         = l_c.uuid_id_,
        .shotgun_id_      = l_c.shotgun_id_,
        .object_id_       = l_c.object_id_,
        .object_type_     = l_c.object_type_,
        .text_            = l_c.text_,
        .data_            = l_c.data_,
        .replies_         = l_c.replies_,
        .checklist_       = l_c.checklist_,
        .pinned_          = l_c.pinned_,
        .links_           = l_c.links,
        .created_at_      = l_c.created_at_,
        .updated_at_      = l_c.updated_at_,
        .task_status_id_  = l_c.task_status_id_,
        .person_id_       = l_c.person_id_,
        .editor_id_       = l_c.editor_id_,
        .preview_file_id_ = l_c.preview_file_id_,
    };
    if (l_map_person.contains(l_c.person_id_))
      l_comment.persons_ = get_comments_t::person_t{
          .uuid_id_    = l_c.person_id_,
          .first_name_ = l_map_person[l_c.person_id_]->first_name_,
          .last_name_  = l_map_person[l_c.person_id_]->last_name_,
          .has_avatar_ = l_map_person[l_c.person_id_]->has_avatar_,
      };
    if (l_map_task_status.contains(l_c.task_status_id_))
      l_comment.task_statuses_ = get_comments_t::task_status_t{
          .uuid_id_    = l_c.task_status_id_,
          .name_       = l_map_task_status[l_c.task_status_id_]->name_,
          .color_      = l_map_task_status[l_c.task_status_id_]->color_,
          .short_name_ = l_map_task_status[l_c.task_status_id_]->short_name_,
      };
    if (l_map_editor.contains(l_c.editor_id_))
      l_comment.editors_ = get_comments_t::person_t{
          .uuid_id_    = l_c.editor_id_,
          .first_name_ = l_map_editor[l_c.editor_id_]->first_name_,
          .last_name_  = l_map_editor[l_c.editor_id_]->last_name_,
          .has_avatar_ = l_map_editor[l_c.editor_id_]->has_avatar_,
      };
    if (l_map_acknowledgements.contains(l_c.uuid_id_))
      l_comment.acknowledgements_ = l_map_acknowledgements[l_c.uuid_id_];
    if (l_map_previews.contains(l_c.uuid_id_)) {
      l_comment.previews_.reserve(l_map_previews.at(l_c.uuid_id_).size());
      for (auto&& i : l_map_previews[l_c.uuid_id_])
        l_comment.previews_.emplace_back(
            get_comments_t::previews_t{
                .uuid_id_           = i->uuid_id_,
                .task_id_           = i->task_id_,
                .revision_          = i->revision_,
                .extension_         = i->extension_,
                .width_             = i->width_,
                .height_            = i->height_,
                .duration_          = i->duration_,
                .status_            = i->status_,
                .validation_status_ = i->validation_status_,
                .original_name_     = i->original_name_,
                .position_          = i->position_,
                .annotations_       = i->annotations_
            }
        );
    }
    if (l_map_mentions.contains(l_c.uuid_id_)) l_comment.mentions_ = l_map_mentions.at(l_c.uuid_id_);
    if (l_map_department_mentions.contains(l_c.uuid_id_))
      l_comment.department_mentions_ = l_map_department_mentions.at(l_c.uuid_id_);
    if (l_map_attachment_file.contains(l_c.uuid_id_)) {
      l_comment.attachment_files_.reserve(l_map_attachment_file.at(l_c.uuid_id_).size());
      for (auto&& i : l_map_attachment_file[l_c.uuid_id_])
        l_comment.attachment_files_.emplace_back(
            get_comments_t::attachment_files_t{
                .uuid_id_   = i->uuid_id_,
                .name_      = i->name_,
                .extension_ = i->extension_,
                .size_      = i->size_,
            }
        );
    }
    l_result.emplace_back(std::move(l_comment));
  }
  return l_result;
}

std::optional<project_task_type_link> sqlite_database::get_project_task_type_link(
    const uuid& in_project_id, const uuid& in_task_type_id
) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<project_task_type_link>(where(
      c(&project_task_type_link::project_id_) == in_project_id &&
      c(&project_task_type_link::task_type_id_) == in_task_type_id
  ));
  return l_t.empty() ? std::nullopt : std::make_optional(l_t.front());
}

std::optional<project_task_status_link> sqlite_database::get_project_task_status_link(
    const uuid& in_project_id, const uuid& in_task_status_uuid
) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<project_task_status_link>(where(
      c(&project_task_status_link::project_id_) == in_project_id &&
      c(&project_task_status_link::task_status_id_) == in_task_status_uuid
  ));
  return l_t.empty() ? std::nullopt : std::make_optional(l_t.front());
}

std::optional<project_asset_type_link> sqlite_database::get_project_asset_type_link(
    const uuid& in_project_id, const uuid& in_asset_type_uuid
) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<project_asset_type_link>(where(
      c(&project_asset_type_link::project_id_) == in_project_id &&
      c(&project_asset_type_link::asset_type_id_) == in_asset_type_uuid
  ));
  return l_t.empty() ? std::nullopt : std::make_optional(l_t.front());
}
bool sqlite_database::is_person_in_project(const person& in_person, const uuid& in_project_id) {
  return is_person_in_project(in_person.uuid_id_, in_project_id);
}
bool sqlite_database::is_person_in_project(const uuid& in_person, const uuid& in_project_id) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.count<project_person_link>(
      where(c(&project_person_link::project_id_) == in_project_id && c(&project_person_link::person_id_) == in_person)
  );
  return l_t > 0;
}

bool sqlite_database::is_task_exist(const uuid& in_entity_id, const uuid& in_task_type_id) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.count<task>(
      where(c(&task::entity_id_) == in_entity_id && c(&task::task_type_id_) == in_task_type_id)
  );
  return l_t > 0;
}
task_status sqlite_database::get_task_status_by_name(const std::string& in_name) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<task_status>(where(c(&task_status::name_) == in_name));
  if (l_t.empty()) throw_exception(doodle_error{"未知的任务状态 {}", in_name});
  return l_t.front();
}

std::set<uuid> sqlite_database::get_person_subscriptions(
    const person& in_person, const uuid& in_project_id, const uuid& in_asset_type_uuid
) {
  using namespace sqlite_orm;
  std::vector<subscription> l_t{};
  l_t = impl_->storage_any_.get_all<subscription>(
      join<task>(on(c(&subscription::task_id_) == c(&task::uuid_id_))),
      join<entity>(on(c(&entity::id_) == c(&task::entity_id_))),
      where(
          c(&subscription::person_id_) == in_person.uuid_id_ &&
          (!in_project_id.is_nil() && c(&task::project_id_) == in_project_id) &&
          (!in_asset_type_uuid.is_nil() && c(&entity::entity_type_id_) == in_asset_type_uuid) &&
          (in_asset_type_uuid.is_nil() && in(&entity::entity_type_id_, get_temporal_type_ids()))
      )
  );
  std::set<uuid> l_result;
  for (auto&& i : l_t) l_result.insert(i.task_id_);
  return l_result;
}

std::vector<entities_and_tasks_t> sqlite_database::get_entities_and_tasks(
    const person& in_person, const uuid& in_project_id, const uuid& in_entity_type_id
) {
  std::vector<entities_and_tasks_t> l_ret{};
  using namespace sqlite_orm;
  auto l_subscriptions_for_user = get_person_subscriptions(in_person, in_project_id, in_entity_type_id);
  auto l_rows                   = impl_->storage_any_.select(
      columns(
          &entity::uuid_id_, &entity::name_, &entity::status_, &entity::uuid_id_, &entity::description_,
          &entity::preview_file_id_, &entity::canceled_, &task::uuid_id_, &task::estimation_, &task::end_date_,
          &task::due_date_, &task::done_date_, &task::duration_, &task::last_comment_date_,
          &task::last_preview_file_id_, &task::priority_, &task::real_start_date_, &task::retake_count_,
          &task::start_date_, &task::difficulty_, &task::task_status_id_, &task::task_type_id_,
          &assignees_table::person_id_
      ),
      join<task>(on(c(&entity::uuid_id_) == c(&task::entity_id_))),
      left_outer_join<assignees_table>(on(c(&assignees_table::task_id_) == c(&task::uuid_id_))),
      where(
          (in_project_id.is_nil() || c(&entity::project_id_) == in_project_id) &&
          (in_entity_type_id.is_nil() || c(&entity::entity_type_id_) == in_entity_type_id)
      )
  );
  std::map<uuid, entities_and_tasks_t> l_entities_and_tasks_map{};
  std::map<uuid, std::size_t> l_task_id_set{};
  for (auto&& [

           uuid_id_, name_, status_, episode_id_, description_, preview_file_id_, canceled_, task_id_, estimation_,
           end_date_, due_date_, done_date_, duration_, last_comment_date_, last_preview_file_id_, priority_,
           real_start_date_, retake_count_, start_date_, difficulty_, task_status_id_, task_type_id_, person_id_

  ] : l_rows) {
    if (!l_entities_and_tasks_map.contains(uuid_id_)) {
      l_entities_and_tasks_map.emplace(
          uuid_id_, entities_and_tasks_t{
                        .uuid_id_         = uuid_id_,
                        .name_            = name_,
                        .status_          = status_,
                        .episode_id_      = episode_id_,
                        .description_     = description_,
                        .preview_file_id_ = preview_file_id_,
                        .canceled_        = canceled_,

                    }
      );
    }
    if (!task_id_.is_nil()) {
      if (!l_task_id_set.contains(task_id_)) {
        l_entities_and_tasks_map[episode_id_].tasks_.emplace_back(
            entities_and_tasks_t::task_t{
                .uuid_id_              = task_id_,
                .estimation_           = estimation_,
                .entity_id_            = episode_id_,
                .end_date_             = end_date_,
                .due_date_             = due_date_,
                .done_date_            = done_date_,
                .duration_             = duration_,
                .last_comment_date_    = last_comment_date_,
                .last_preview_file_id_ = last_preview_file_id_,
                .priority_             = priority_,
                .real_start_date_      = real_start_date_,
                .retake_count_         = retake_count_,
                .start_date_           = start_date_,
                .difficulty_           = difficulty_,
                .task_status_id_       = task_status_id_,
                .task_type_id_         = task_type_id_,
                .is_subscribed_        = l_subscriptions_for_user.contains(task_id_),
            }
        );
        l_task_id_set.emplace(task_id_, l_entities_and_tasks_map[episode_id_].tasks_.size() - 1);
      }
      if (!person_id_.is_nil())
        l_entities_and_tasks_map[episode_id_].tasks_[l_task_id_set.at(task_id_)].assigners_.emplace_back(person_id_);
    }
  }
  l_ret = l_entities_and_tasks_map | ranges::views::values | ranges::to_vector;
  return l_ret;
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
std::vector<working_file> sqlite_database::get_working_file_by_task(const uuid& in_task_id) {
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.get_all<working_file>(
      join<working_file_task_link>(on(c(&working_file_task_link::working_file_id_) == c(&working_file::uuid_id_))),
      where(c(&working_file_task_link::task_id_) == in_task_id)
  );
  return l_t;
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
  auto l_t = impl_->storage_any_.get_all<playlist_shot>(where(c(&playlist_shot::playlist_id_) == in_playlist_id));
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

boost::asio::awaitable<void> sqlite_database::delete_working_file_orphaned() {
  DOODLE_TO_SQLITE_THREAD_2();
  using namespace sqlite_orm;

  impl_->storage_any_.remove_all<working_file>(where(not_in(
      &working_file::uuid_id_,
      union_(select(&working_file_task_link::working_file_id_), select(&working_file_entity_link::working_file_id_))
  )));
  DOODLE_TO_SELF();
  co_return;
}

DOODLE_GET_BY_PARENT_ID_SQL(assets_helper::database_t);

DOODLE_UUID_TO_ID(assets_file_helper::database_t)
DOODLE_UUID_TO_ID(assets_helper::database_t)
DOODLE_UUID_TO_ID(person)
DOODLE_UUID_TO_ID(entity)
DOODLE_UUID_TO_ID(working_file)
DOODLE_UUID_TO_ID(attendance_helper::database_t)
DOODLE_UUID_TO_ID(asset_type)
DOODLE_UUID_TO_ID(ai_image_metadata)
DOODLE_UUID_TO_ID(project)
DOODLE_UUID_TO_ID(task)

DOODLE_ID_TO_UUID(assets_file_helper::database_t)
DOODLE_ID_TO_UUID(assets_helper::database_t)
DOODLE_ID_TO_UUID(working_file)
DOODLE_ID_TO_UUID(attendance_helper::database_t)
DOODLE_ID_TO_UUID(notification)

template <>
person sqlite_database::get_by_uuid<person>(const uuid& in_uuid) {
  auto l_p         = impl_->get_by_uuid<person>(in_uuid);
  l_p.departments_ = impl_->storage_any_.select(
      &person_department_link::department_id_,
      sqlite_orm::where(sqlite_orm::c(&person_department_link::person_id_) == in_uuid)
  );
  return l_p;
}

DOODLE_GET_BY_UUID_SQL(preview_file)
DOODLE_GET_BY_UUID_SQL(attachment_file)
template <>
assets_file_helper::database_t sqlite_database::get_by_uuid<assets_file_helper::database_t>(const uuid& in_uuid) {
  using namespace sqlite_orm;
  auto l_data          = impl_->get_by_uuid<assets_file_helper::database_t>(in_uuid);
  l_data.uuid_parents_ = impl_->storage_any_.select(
      &assets_file_helper::link_parent_t::assets_type_uuid_,
      where(c(&assets_file_helper::link_parent_t::assets_uuid_) == in_uuid)
  );
  return l_data;
}
DOODLE_GET_BY_UUID_SQL(assets_helper::database_t)
DOODLE_GET_BY_UUID_SQL(server_task_info)
DOODLE_GET_BY_UUID_SQL(project_status)
DOODLE_GET_BY_UUID_SQL(task_type)
DOODLE_GET_BY_UUID_SQL(asset_type)
DOODLE_GET_BY_UUID_SQL(department)
DOODLE_GET_BY_UUID_SQL(task_status)
DOODLE_GET_BY_UUID_SQL(notification)
DOODLE_GET_BY_UUID_SQL(comment)
DOODLE_GET_BY_UUID_SQL(playlist)
DOODLE_GET_BY_UUID_SQL(working_file)
DOODLE_GET_BY_UUID_SQL(ai_image_metadata)
DOODLE_GET_BY_UUID_SQL(organisation)
DOODLE_GET_BY_UUID_SQL(playlist_shot)
template <>
task sqlite_database::get_by_uuid<task>(const uuid& in_uuid) {
  using namespace sqlite_orm;
  auto l_ret = impl_->get_by_uuid<task>(in_uuid);
  l_ret.assignees_ =
      impl_->storage_any_.select(&assignees_table::person_id_, where(c(&assignees_table::task_id_) == in_uuid));
  return l_ret;
}
DOODLE_GET_BY_UUID_SQL(work_xlsx_task_info_helper::database_t)
template <>
entity sqlite_database::get_by_uuid<entity>(const uuid& in_uuid) {
  using namespace sqlite_orm;
  auto l_ret                  = impl_->get_by_uuid<entity>(in_uuid);
  l_ret.entities_out          = impl_->storage_any_.select(&entity::uuid_id_, where(c(&entity::parent_id_) == in_uuid));
  l_ret.entity_concept_links_ = impl_->storage_any_.select(
      &entity_concept_link::entity_id_, where(c(&entity_concept_link::entity_id_) == in_uuid)
  );
  return l_ret;
  // l_ret.instance_casting_ = impl_->storage_any_.select(
  //     &asset_instance::instance_id_,
  //     where(c(&asset_instance::entity_id_) == in_uuid)
  // )
}
template <>
project sqlite_database::get_by_uuid<project>(const uuid& in_uuid) {
  using namespace sqlite_orm;
  auto l_prj  = impl_->get_by_uuid<project>(in_uuid);
  l_prj.team_ = impl_->storage_any_.select(
      &project_person_link::person_id_, where(c(&project_person_link::project_id_) == in_uuid)
  );
  l_prj.asset_types_ = impl_->storage_any_.select(
      &project_asset_type_link::asset_type_id_, where(c(&project_asset_type_link::project_id_) == in_uuid)
  );
  l_prj.task_statuses_ = impl_->storage_any_.select(
      &project_task_status_link::uuid_id_, where(c(&project_task_status_link::project_id_) == in_uuid)
  );
  l_prj.task_types_ = impl_->storage_any_.select(
      &project_task_type_link::task_type_id_, where(c(&project_task_type_link::project_id_) == in_uuid)
  );
  l_prj.status_automations_ = impl_->storage_any_.select(
      &project_status_automation_link::status_automation_id_,
      where(c(&project_status_automation_link::project_id_) == in_uuid)
  );
  l_prj.preview_background_files_ = impl_->storage_any_.select(
      &project_preview_background_file_link::preview_background_file_id_,
      where(c(&project_preview_background_file_link::project_id_) == in_uuid)
  );
  return l_prj;
}
DOODLE_GET_BY_UUID_SQL(attendance_helper::database_t)

template <>
std::vector<assets_file_helper::database_t> sqlite_database::get_all() {
  auto l_list = impl_->get_all<assets_file_helper::database_t>();
  auto l_map  = l_list |
               ranges::views::transform(
                   [](assets_file_helper::database_t& i) -> std::pair<uuid, assets_file_helper::database_t*> {
                     return {i.uuid_id_, &i};
                   }
               ) |
               ranges::to<std::map<uuid, assets_file_helper::database_t*>>();
  auto l_link = impl_->get_all<assets_file_helper::link_parent_t>();
  for (auto&& i : l_link) {
    if (l_map.contains(i.assets_uuid_)) l_map.at(i.assets_uuid_)->uuid_parents_.emplace_back(i.assets_type_uuid_);
  }
  return l_list;
}
DOODLE_GET_ALL_SQL(assets_helper::database_t)
DOODLE_GET_ALL_SQL(server_task_info)
DOODLE_GET_ALL_SQL(project_status)
DOODLE_GET_ALL_SQL(task_status)
DOODLE_GET_ALL_SQL(task_type)
DOODLE_GET_ALL_SQL(department)
DOODLE_GET_ALL_SQL(studio)
DOODLE_GET_ALL_SQL(working_file)
DOODLE_GET_ALL_SQL(status_automation)
DOODLE_GET_ALL_SQL(organisation)
DOODLE_GET_ALL_SQL(ai_image_metadata)
DOODLE_GET_ALL_SQL(project)
template <>
std::vector<asset_type> sqlite_database::get_all() {
  auto l_list = impl_->get_all<asset_type>();
  auto l_map =
      l_list |
      ranges::views::transform([](asset_type& i) -> std::pair<uuid, asset_type*> { return {i.uuid_id_, &i}; }) |
      ranges::to<std::map<uuid, asset_type*>>();
  auto l_link = impl_->get_all<task_type_asset_type_link>();
  for (auto&& i : impl_->get_all<task_type_asset_type_link>()) {
    if (l_map.contains(i.asset_type_id_)) l_map.at(i.asset_type_id_)->task_types_.emplace_back(i.task_type_id_);
  }
  return l_list;
}
template <>
std::vector<person> sqlite_database::get_all() {
  auto l_list = impl_->get_all<person>();
  for (auto&& i : l_list) {
    i.departments_ = impl_->storage_any_.select(
        &person_department_link::department_id_,
        sqlite_orm::where(sqlite_orm::c(&person_department_link::person_id_) == i.uuid_id_)
    );
  }
  return l_list;
}

DOODLE_INSTALL_SQL(assets_file_helper::database_t)
DOODLE_INSTALL_SQL(assets_helper::database_t)
DOODLE_INSTALL_SQL(attachment_file)
DOODLE_INSTALL_SQL(comment_mentions)
DOODLE_INSTALL_SQL(comment_department_mentions)
DOODLE_INSTALL_SQL(notification)
DOODLE_INSTALL_SQL(server_task_info)
DOODLE_INSTALL_SQL(work_xlsx_task_info_helper::database_t)
DOODLE_INSTALL_SQL(project)
DOODLE_INSTALL_SQL(project_status)
DOODLE_INSTALL_SQL(task_status)
DOODLE_INSTALL_SQL(task_type)
DOODLE_INSTALL_SQL(asset_type)
DOODLE_INSTALL_SQL(task_type_asset_type_link)
DOODLE_INSTALL_SQL(project_task_type_link)
DOODLE_INSTALL_SQL(project_task_status_link)
DOODLE_INSTALL_SQL(department)
DOODLE_INSTALL_SQL(entity_asset_extend)
DOODLE_INSTALL_SQL(person)
DOODLE_INSTALL_SQL(assignees_table)
DOODLE_INSTALL_SQL(preview_file)
DOODLE_INSTALL_SQL(comment_preview_link)
DOODLE_INSTALL_SQL(task)
DOODLE_INSTALL_SQL(comment)
DOODLE_INSTALL_SQL(project_asset_type_link)
DOODLE_INSTALL_SQL(playlist_shot)
DOODLE_INSTALL_SQL(entity)
DOODLE_INSTALL_SQL(project_person_link)
DOODLE_INSTALL_SQL(comment_acknoledgments)
DOODLE_INSTALL_SQL(attendance_helper::database_t)
DOODLE_INSTALL_SQL(assets_file_helper::link_parent_t)
DOODLE_INSTALL_SQL(working_file)
DOODLE_INSTALL_SQL(playlist)
DOODLE_INSTALL_SQL(ai_image_metadata)
DOODLE_INSTALL_SQL(project_status_automation_link)
DOODLE_INSTALL_SQL(entity_shot_extend)

DOODLE_INSTALL_RANGE(attendance_helper::database_t)
DOODLE_INSTALL_RANGE(work_xlsx_task_info_helper::database_t)
DOODLE_INSTALL_RANGE(assets_helper::database_t)
DOODLE_INSTALL_RANGE(assets_file_helper::database_t)
DOODLE_INSTALL_RANGE(comment_mentions)
DOODLE_INSTALL_RANGE(comment_department_mentions)
DOODLE_INSTALL_RANGE(notification)
DOODLE_INSTALL_RANGE(task_status)
DOODLE_INSTALL_RANGE(task_type)
DOODLE_INSTALL_RANGE(asset_type)
DOODLE_INSTALL_RANGE(assets_file_helper::link_parent_t)
DOODLE_INSTALL_RANGE(task)
DOODLE_INSTALL_RANGE(person_department_link)
DOODLE_INSTALL_RANGE(working_file)
DOODLE_INSTALL_RANGE(entity_link)
DOODLE_INSTALL_RANGE(working_file_task_link)
DOODLE_INSTALL_RANGE(working_file_entity_link)
DOODLE_INSTALL_RANGE(playlist_shot)
DOODLE_INSTALL_RANGE(task_type_asset_type_link)
DOODLE_INSTALL_RANGE(assignees_table)
DOODLE_INSTALL_RANGE(entity_shot_extend)

DOODLE_REMOVE_BY_ID(attendance_helper::database_t)
DOODLE_REMOVE_BY_ID(work_xlsx_task_info_helper::database_t)
DOODLE_REMOVE_BY_ID(assets_file_helper::database_t)
DOODLE_REMOVE_BY_ID(assets_helper::database_t)
DOODLE_REMOVE_BY_ID(assignees_table)
DOODLE_REMOVE_BY_ID(comment)
DOODLE_REMOVE_BY_ID(assets_file_helper::link_parent_t)
DOODLE_REMOVE_BY_ID(comment_acknoledgments)
DOODLE_REMOVE_BY_ID(project_person_link)
DOODLE_REMOVE_BY_ID(working_file_task_link)
DOODLE_REMOVE_BY_ID(working_file)
DOODLE_REMOVE_BY_ID(entity_link)
DOODLE_REMOVE_BY_ID(playlist)
DOODLE_REMOVE_BY_ID(playlist_shot)
DOODLE_REMOVE_BY_ID(entity_shot_extend)
DOODLE_REMOVE_BY_ID(project_status_automation_link)

DOODLE_REMOVE_BY_UUID(attendance_helper::database_t)
DOODLE_REMOVE_BY_UUID(work_xlsx_task_info_helper::database_t)
DOODLE_REMOVE_BY_UUID(assets_file_helper::database_t)
DOODLE_REMOVE_BY_UUID(assets_helper::database_t)
DOODLE_REMOVE_BY_UUID(working_file)
DOODLE_REMOVE_BY_UUID(playlist)
DOODLE_REMOVE_BY_UUID(entity_link)
DOODLE_REMOVE_BY_UUID(ai_image_metadata)
DOODLE_REMOVE_BY_UUID(playlist_shot)

template <>
boost::asio::awaitable<void> sqlite_database::remove<task>(const std::vector<uuid>& in_data) {
  using namespace sqlite_orm;

  auto l_comments = impl_->storage_any_.select(&comment::uuid_id_, where(in(&comment::object_id_, in_data)));
  auto l_subscription =
      impl_->storage_any_.select(&subscription::uuid_id_, where(in(&subscription::task_id_, in_data)));
  auto l_preview_file =
      impl_->storage_any_.select(&preview_file::uuid_id_, where(in(&preview_file::task_id_, in_data)));
  l_preview_file |= ranges::actions::push_back(impl_->storage_any_.select(
      &comment_preview_link::preview_file_id_, where(in(&comment_preview_link::comment_id_, l_comments))
  ));
  auto l_notification =
      impl_->storage_any_.select(&notification::uuid_id_, where(in(&notification::task_id_, in_data)));

  auto l_attachments =
      impl_->storage_any_.select(&attachment_file::uuid_id_, where(in(&attachment_file::comment_id_, l_comments)));
  auto l_assignees = impl_->storage_any_.select(&assignees_table::id_, where(in(&assignees_table::task_id_, in_data)));
  DOODLE_TO_SQLITE_THREAD_2();
  auto l_g = impl_->storage_any_.transaction_guard();

  impl_->storage_any_.update_all(
      set(c(&entity::preview_file_id_) = null()), where(in(&entity::preview_file_id_, l_preview_file))
  );
  impl_->storage_any_.remove_all<comment_preview_link>(where(in(&comment_preview_link::comment_id_, l_comments)));
  impl_->storage_any_.remove_all<comment_mentions>(where(in(&comment_mentions::comment_id_, l_comments)));
  impl_->storage_any_.remove_all<comment_department_mentions>(
      where(in(&comment_department_mentions::comment_id_, l_comments))
  );
  impl_->storage_any_.remove_all<comment_acknoledgments>(where(in(&comment_acknoledgments::comment_id_, l_comments)));
  impl_->storage_any_.remove_all<notification>(where(in(&notification::uuid_id_, l_notification)));
  impl_->storage_any_.remove_all<notification>(where(in(&notification::comment_id_, l_comments)));
  impl_->storage_any_.remove_all<preview_file>(where(in(&preview_file::uuid_id_, l_preview_file)));
  impl_->storage_any_.remove_all<attachment_file>(where(in(&attachment_file::uuid_id_, l_attachments)));
  impl_->storage_any_.remove_all<comment>(where(in(&comment::uuid_id_, l_comments)));

  impl_->storage_any_.remove_all<subscription>(where(in(&subscription::uuid_id_, l_subscription)));
  impl_->storage_any_.remove_all<assignees_table>(where(in(&assignees_table::id_, l_assignees)));
  impl_->storage_any_.remove_all<task>(where(in(&task::uuid_id_, in_data)));
  l_g.commit();

  DOODLE_TO_SELF();
}

template <>
boost::asio::awaitable<void> sqlite_database::remove<task>(const uuid& in_data) {
  using namespace sqlite_orm;

  auto l_comments = impl_->storage_any_.select(&comment::uuid_id_, where(c(&comment::object_id_) == in_data));
  auto l_subscription =
      impl_->storage_any_.select(&subscription::uuid_id_, where(c(&subscription::task_id_) == in_data));
  auto l_preview_file =
      impl_->storage_any_.select(&preview_file::uuid_id_, where(c(&preview_file::task_id_) == in_data));
  auto l_notification =
      impl_->storage_any_.select(&notification::uuid_id_, where(c(&notification::task_id_) == in_data));
  l_preview_file |= ranges::actions::push_back(impl_->storage_any_.select(
      &comment_preview_link::preview_file_id_, where(in(&comment_preview_link::comment_id_, l_comments))
  ));
  auto l_attachments =
      impl_->storage_any_.select(&attachment_file::uuid_id_, where(in(&attachment_file::comment_id_, l_comments)));
  auto l_assignees = impl_->storage_any_.select(&assignees_table::id_, where(c(&assignees_table::task_id_) == in_data));
  DOODLE_TO_SQLITE_THREAD_2();
  auto l_g = impl_->storage_any_.transaction_guard();
  impl_->storage_any_.update_all(
      set(c(&entity::preview_file_id_) = null()), where(in(&entity::preview_file_id_, l_preview_file))
  );

  impl_->storage_any_.remove_all<comment_preview_link>(where(in(&comment_preview_link::comment_id_, l_comments)));
  impl_->storage_any_.remove_all<comment_mentions>(where(in(&comment_mentions::comment_id_, l_comments)));
  impl_->storage_any_.remove_all<comment_department_mentions>(
      where(in(&comment_department_mentions::comment_id_, l_comments))
  );
  impl_->storage_any_.remove_all<comment_acknoledgments>(where(in(&comment_acknoledgments::comment_id_, l_comments)));
  impl_->storage_any_.remove_all<notification>(where(in(&notification::uuid_id_, l_notification)));
  impl_->storage_any_.remove_all<notification>(where(in(&notification::comment_id_, l_comments)));
  impl_->storage_any_.remove_all<preview_file>(where(in(&preview_file::uuid_id_, l_preview_file)));
  impl_->storage_any_.remove_all<attachment_file>(where(in(&attachment_file::uuid_id_, l_attachments)));
  impl_->storage_any_.remove_all<comment>(where(in(&comment::uuid_id_, l_comments)));

  impl_->storage_any_.remove_all<subscription>(where(in(&subscription::uuid_id_, l_subscription)));
  impl_->storage_any_.remove_all<assignees_table>(where(in(&assignees_table::id_, l_assignees)));
  impl_->storage_any_.remove_all<task>(where(c(&task::uuid_id_) == in_data));
  l_g.commit();

  DOODLE_TO_SELF();
}
template <>
boost::asio::awaitable<void> sqlite_database::remove<entity>(const uuid& in_data) {
  using namespace sqlite_orm;
  DOODLE_TO_SQLITE_THREAD_2();
  auto l_g = impl_->storage_any_.transaction_guard();
  impl_->storage_any_.remove_all<entity_asset_extend>(where(c(&entity_asset_extend::entity_id_) == in_data));
  impl_->storage_any_.remove_all<entity_link>(where(c(&entity_link::entity_in_id_) == in_data));
  impl_->storage_any_.remove_all<entity_link>(where(c(&entity_link::entity_out_id_) == in_data));
  impl_->storage_any_.remove_all<entity_concept_link>(where(c(&entity_concept_link::entity_out_id_) == in_data));
  impl_->storage_any_.remove_all<entity_concept_link>(where(c(&entity_concept_link::entity_out_id_) == in_data));
  impl_->storage_any_.remove_all<subscription>(where(c(&subscription::entity_id_) == in_data));
  impl_->storage_any_.remove_all<entity>(where(c(&entity::uuid_id_) == in_data));
  l_g.commit();
  DOODLE_TO_SELF();

  co_return;
}
template <>
boost::asio::awaitable<void> sqlite_database::remove<comment>(const uuid& in_data) {
  using namespace sqlite_orm;
  auto l_previews = impl_->storage_any_.select(
      &comment_preview_link::preview_file_id_, where(c(&comment_preview_link::comment_id_) == in_data)
  );
  auto l_attachments =
      impl_->storage_any_.select(&attachment_file::uuid_id_, where(c(&attachment_file::comment_id_) == in_data));

  DOODLE_TO_SQLITE_THREAD_2();
  auto l_g = impl_->storage_any_.transaction_guard();
  impl_->storage_any_.update_all(
      set(c(&comment::preview_file_id_) = null()), where(in(&comment::preview_file_id_, l_previews))
  );
  impl_->storage_any_.update_all(
      set(c(&entity::preview_file_id_) = null()), where(in(&entity::preview_file_id_, l_previews))
  );
  impl_->storage_any_.remove_all<comment_preview_link>(where(c(&comment_preview_link::comment_id_) == in_data));
  impl_->storage_any_.remove_all<comment_preview_link>(where(in(&comment_preview_link::preview_file_id_, l_previews)));
  impl_->storage_any_.remove_all<comment_mentions>(where(c(&comment_mentions::comment_id_) == in_data));
  impl_->storage_any_.remove_all<comment_department_mentions>(
      where(c(&comment_department_mentions::comment_id_) == in_data)
  );
  impl_->storage_any_.remove_all<comment_acknoledgments>(where(c(&comment_acknoledgments::comment_id_) == in_data));

  impl_->storage_any_.remove_all<notification>(where(c(&notification::comment_id_) == in_data));
  impl_->storage_any_.remove_all<preview_file>(where(in(&preview_file::uuid_id_, l_previews)));
  impl_->storage_any_.remove_all<attachment_file>(where(in(&attachment_file::uuid_id_, l_attachments)));

  impl_->storage_any_.remove_all<comment>(where(c(&comment::uuid_id_) == in_data));
  l_g.commit();
  DOODLE_TO_SELF();
}
DOODLE_REMOVE_BY_UUID(server_task_info)
DOODLE_REMOVE_BY_UUID(project_task_status_link)

}  // namespace doodle