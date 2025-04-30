//
// Created by TD on 24-9-12.
//

#include "sqlite_database.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/asset_instance.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/kitsu/assets_type.h>
#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

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

}  // namespace
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

std::int32_t sqlite_database::get_notification_count(const uuid& in_user_id) {
  return impl_->get_notification_count(in_user_id);
}

std::vector<project_with_extra_data> sqlite_database::get_project_for_user(const person& in_user) {
  std::vector<project_with_extra_data> l_projects{};
  if (in_user.role_ == person_role_type::admin) {
    auto l_t   = impl_->get_all<project>();
    l_projects = l_t | ranges::views::transform([](const project& in) { return project_with_extra_data{in}; }) |
                 ranges::to_vector;
  } else {
    auto l_t   = get_person_projects(in_user);
    l_projects = l_t | ranges::views::transform([](const project& in) { return project_with_extra_data{in}; }) |
                 ranges::to_vector;
  }
  auto l_descriptors = impl_->storage_any_.get_all<metadata_descriptor>();
  {
    // l_descriptors |= ranges::actions::unique([](const metadata_descriptor& in_r, const metadata_descriptor& in_l) {
    //   return in_r.uuid_id_ == in_l.uuid_id_;
    // });
    auto l_metadata_descriptor_department_link = impl_->storage_any_.get_all<metadata_descriptor_department_link>();

    auto l_metadata_descriptor_department_link_map =
        l_metadata_descriptor_department_link |
        ranges::views::transform([](const metadata_descriptor_department_link& in) {
          return std::make_pair(in.metadata_descriptor_uuid_, in);
        }) |
        ranges::to<std::map<uuid, metadata_descriptor_department_link>>;
    for (auto&& i : l_descriptors) {
      if (l_metadata_descriptor_department_link_map.contains(i.uuid_id_)) {
        i.department_.emplace_back(l_metadata_descriptor_department_link_map.at(i.uuid_id_).department_uuid_);
      }
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
      sqlite_orm::left_join<project_status>(
          sqlite_orm::on(sqlite_orm::c(&project_status::uuid_id_) == sqlite_orm::c(&project::project_status_id_))
      ),
      sqlite_orm::left_join<project_person_link>(
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
static constexpr auto sql_orm_todo_t = sqlite_orm::struct_<todo_t>(
    &task::uuid_id_,               //
    &task::name_,                  //
    &task::description_,           //
    &task::priority_,              //
    &task::difficulty_,            //
    &task::duration_,              //
    &task::estimation_,            //
    &task::completion_rate_,       //
    &task::retake_count_,          //
    &task::sort_order_,            //
    &task::start_date_,            //
    &task::due_date_,              //
    &task::real_start_date_,       //
    &task::end_date_,              //
    &task::done_date_,             //
    &task::last_comment_date_,     //
    &task::nb_assets_ready_,       //
    &task::data_,                  //
    &task::shotgun_id_,            //
    &task::last_preview_file_id_,  //
    &task::project_id_,            //
    &task::task_type_id_,          //
    &task::task_status_id_,        //
    &task::assigner_id_,           //
    &task::created_at_,            //
    &task::updated_at_,            //
    &project::name_,               //
    &project::has_avatar_,         //
    &entity::uuid_id_,             //
    &entity::name_,                //
    &entity::description_,         //
    &entity::data_,                //
    &entity::preview_file_id_,     //
    &asset_type::name_,            //
    &entity::canceled_,            //
    &entity::parent_id_,           //
    &entity::source_id_,           //
    &task_type::name_,             //
    &task_type::for_entity_,       //
    &task_type::color_,            //
    &task_status::name_,           //
    &task_status::color_,          //
    &task_status::short_name_      //
);

void sqlite_database::todo_post_processing(std::vector<todo_t>& in_tasks) {
  auto l_task_ids =
      in_tasks | ranges::views::transform([](const todo_t& in) { return in.uuid_id_; }) | ranges::to_vector;

  {
    std::map<uuid, const comment*> l_map_comm;
    auto l_comms = impl_->storage_any_.get_all<comment>(
        sqlite_orm::where(sqlite_orm::in(&comment::object_id_, l_task_ids)), sqlite_orm::order_by(&comment::created_at_)
    );
    for (auto&& i : l_comms) {
      if (!l_map_comm.contains(i.object_id_)) l_map_comm[i.object_id_] = &i;
    }

    for (auto& i : in_tasks) {
      if (l_map_comm.contains(i.uuid_id_)) {
        auto&& l_c = l_map_comm.at(i.uuid_id_);
        i.last_comment_ =
            todo_t::comment_t{.text_ = l_c->text_, .date_ = l_c->created_at_, .person_id_ = l_c->person_id_};
      }
    }
  }
  std::map<uuid, todo_t*> l_map_task;
  for (auto&& i : in_tasks) l_map_task[i.uuid_id_] = &i;

  for (auto l_ass = impl_->storage_any_.get_all<assignees_table>(
           sqlite_orm::where(sqlite_orm::in(&assignees_table::task_id_, l_task_ids))
       );
       auto&& i : l_ass) {
    if (l_map_task.contains(i.task_id_)) l_map_task.at(i.task_id_)->assignees_.emplace_back(i.person_id_);
  }
}

std::vector<todo_t> sqlite_database::get_person_tasks(const person& in_user, bool is_done) {
  using namespace sqlite_orm;
  auto l_prjs    = get_person_projects(in_user);
  auto l_pej_ids = l_prjs | ranges::views::transform([](const project& in) { return in.uuid_id_; }) | ranges::to_vector;
  auto l_order_by = dynamic_order_by(impl_->storage_any_);
  if (is_done) {
    l_order_by.push_back(order_by(&task::end_date_).desc());
    l_order_by.push_back(order_by(&task_type::name_));
    l_order_by.push_back(order_by(&entity::name_));
  } else
    l_order_by.push_back(order_by(&entity::name_));

  auto l_task = impl_->storage_any_.select(
      sql_orm_todo_t,  //
      join<project>(on(c(&task::project_id_) == c(&project::uuid_id_))),
      join<task_type>(on(c(&task::task_type_id_) == c(&task_type::uuid_id_))),
      join<task_status>(on(c(&task::task_status_id_) == c(&task_status::uuid_id_))),
      join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
      join<assignees_table>(on(c(&task::uuid_id_) == c(&assignees_table::task_id_))),
      join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
      where(
          in(&task::project_id_, l_pej_ids) && c(&assignees_table::person_id_) == in_user.uuid_id_ &&
          c(&task_status::is_done_) == is_done
      ),
      l_order_by
  );
  todo_post_processing(l_task);
  return l_task;
}
std::vector<todo_t> sqlite_database::get_preson_tasks_to_check(const person& in_user) {
  using namespace sqlite_orm;
  auto l_prjs    = in_user.role_ == person_role_type::admin ? impl_->get_all<project>() : get_person_projects(in_user);
  auto l_pej_ids = l_prjs | ranges::views::transform([](const project& in) { return in.uuid_id_; }) | ranges::to_vector;

  auto l_task    = in_user.role_ == person_role_type::supervisor
                       ? impl_->storage_any_.select(
                          sql_orm_todo_t,  //
                          join<project>(on(c(&task::project_id_) == c(&project::uuid_id_))),
                          join<task_type>(on(c(&task::task_type_id_) == c(&task_type::uuid_id_))),
                          join<task_status>(on(c(&task::task_status_id_) == c(&task_status::uuid_id_))),
                          join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
                          join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
                          where(
                              c(&task_status::is_feedback_request_) && in(&task::project_id_, l_pej_ids) &&
                              in(&task_type::department_id_, in_user.departments_)
                          )
                      )
                       : impl_->storage_any_.select(
                          sql_orm_todo_t,  //
                          join<project>(on(c(&task::project_id_) == c(&project::uuid_id_))),
                          join<task_type>(on(c(&task::task_type_id_) == c(&task_type::uuid_id_))),
                          join<task_status>(on(c(&task::task_status_id_) == c(&task_status::uuid_id_))),
                          join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
                          join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
                          where(c(&task_status::is_feedback_request_) && in(&task::project_id_, l_pej_ids))
                      );
  todo_post_processing(l_task);
  return l_task;
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
      where(!in_user.uuid_id_.is_nil() && c(&project_person_link::person_id_) == in_user.uuid_id_)
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
  auto l_acknowledgements =
      impl_->storage_any_.get_all<comment_acknoledgments>(where(in(&comment_acknoledgments::comment_id_, l_comment_ids))
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
      join<comment_preview_link>(on(c(&comment_preview_link::comment_id_) == c(&comment::uuid_id_))),
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
  using namespace sqlite_orm;
  auto l_t = impl_->storage_any_.count<project_person_link>(where(
      c(&project_person_link::project_id_) == in_project_id && c(&project_person_link::person_id_) == in_person.uuid_id_
  ));
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

std::vector<assets_and_tasks_t> sqlite_database::get_assets_and_tasks(
    const person& in_person, const uuid& in_project_id, const uuid& in_id
) {
  using namespace sqlite_orm;
  struct assets_and_tasks_tmp_t {
    decltype(entity::uuid_id_) uuid_id_;
    decltype(entity::name_) name_;
    decltype(entity::preview_file_id_) preview_file_id_;
    decltype(entity::description_) description_;
    decltype(asset_type::name_) asset_type_name_;

    decltype(asset_type::uuid_id_) asset_type_uuid_id_;
    decltype(entity::canceled_) canceled_;
    decltype(entity::ready_for_) ready_for_;
    decltype(entity::source_id_) source_id_;

    decltype(entity::is_casting_standby_) is_casting_standby_;
    decltype(entity::is_shared_) is_shared_;
    decltype(entity::data_) data_;
    decltype(task::uuid_id_) task_uuid_id_;
    decltype(task::due_date_) due_date_;

    decltype(task::done_date_) done_date_;
    decltype(task::duration_) duration_;
    decltype(task::entity_id_) entity_id_;
    decltype(task::estimation_) estimation_;
    decltype(task::end_date_) end_date_;

    decltype(task::last_comment_date_) last_comment_date_;
    decltype(task::last_preview_file_id_) last_preview_file_id_;
    decltype(task::priority_) priority_;
    decltype(task::real_start_date_) real_start_date_;

    decltype(task::retake_count_) retake_count_;
    decltype(task::start_date_) start_date_;
    decltype(task::difficulty_) difficulty_;
    decltype(task::task_type_id_) task_type_id_;
    decltype(task::task_status_id_) task_status_id_;

    decltype(task::assigner_id_) assigner_id_;
  };
  constexpr auto l_sql_orm = struct_<assets_and_tasks_tmp_t>(
      &entity::uuid_id_, &entity::name_, &entity::preview_file_id_, &entity::description_, &asset_type::name_,
      &asset_type::uuid_id_, &entity::canceled_, &entity::ready_for_, &entity::source_id_, &entity::is_casting_standby_,
      &entity::is_shared_, &entity::data_, &task::uuid_id_, &task::due_date_, &task::done_date_, &task::duration_,
      &task::entity_id_, &task::estimation_, &task::end_date_, &task::last_comment_date_, &task::last_preview_file_id_,
      &task::priority_, &task::real_start_date_, &task::retake_count_, &task::start_date_, &task::difficulty_,
      &task::task_type_id_, &task::task_status_id_, &task::assigner_id_
  );
  std::vector<assets_and_tasks_tmp_t> l_t{};
  auto l_subscriptions_for_user = get_person_subscriptions(in_person, in_project_id, {});
  auto l_temporal_type_ids      = get_temporal_type_ids();
  l_t                           = impl_->storage_any_.select(
      l_sql_orm, join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
      left_outer_join<task>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
      left_outer_join<assignees_table>(on(c(&assignees_table::task_id_) == c(&task::uuid_id_))),
      where(
          ((!in_id.is_nil() && c(&entity::uuid_id_) == in_id) ||
           (!in_project_id.is_nil() && c(&entity::project_id_) == in_project_id)) &&
          sqlite_orm::not_in(&entity::entity_type_id_, l_temporal_type_ids)
      ),
      multi_order_by(order_by(&asset_type::name_), order_by(&entity::name_))
  );
  std::map<uuid, assets_and_tasks_t> l_ret{};
  std::map<uuid, std::size_t> l_task_id_set{};
  for (auto&& i : l_t) {
    if (!l_ret.contains(i.uuid_id_))
      l_ret[i.uuid_id_] = assets_and_tasks_t{
          .uuid_id_            = i.uuid_id_,
          .name_               = i.name_,
          .preview_file_id_    = i.preview_file_id_,
          .description_        = i.description_,
          .asset_type_name_    = i.asset_type_name_,
          .asset_type_id_      = i.asset_type_uuid_id_,
          .canceled_           = i.canceled_,
          .ready_for_          = i.ready_for_,
          .source_id_          = i.source_id_,
          .is_casting_standby_ = i.is_casting_standby_,
          .is_shared_          = i.is_shared_,
          .data_               = i.data_,
      };
    if (i.task_uuid_id_.is_nil()) continue;
    if (!l_task_id_set.contains(i.task_uuid_id_)) {
      l_ret[i.uuid_id_].tasks_.emplace_back(
          assets_and_tasks_t::task_t{
              .uuid_id_              = i.task_uuid_id_,
              .due_date_             = i.due_date_,
              .done_date_            = i.done_date_,
              .duration_             = i.duration_,
              .entity_id_            = i.entity_id_,
              .estimation_           = i.estimation_,
              .end_date_             = i.end_date_,
              .last_comment_date_    = i.last_comment_date_,
              .last_preview_file_id_ = i.last_preview_file_id_,
              .priority_             = i.priority_,
              .real_start_date_      = i.real_start_date_,
              .retake_count_         = i.retake_count_,
              .start_date_           = i.start_date_,
              .difficulty_           = i.difficulty_,
              .task_type_id_         = i.task_type_id_,
              .task_status_id_       = i.task_status_id_,
          }
      );
      l_task_id_set.emplace(i.task_uuid_id_, l_ret[i.uuid_id_].tasks_.size() - 1);
    }
    if (!i.assigner_id_.is_nil())
      l_ret[i.uuid_id_].tasks_[l_task_id_set.at(i.task_uuid_id_)].assigner_ids_.emplace_back(i.assigner_id_);
  }
  return l_ret | ranges::views::values | ranges::to_vector;
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
          &entity::preview_file_id_, &entity::canceled_, &entity::data_, &task::uuid_id_, &task::estimation_,
          &task::end_date_, &task::due_date_, &task::done_date_, &task::duration_, &task::last_comment_date_,
          &task::last_preview_file_id_, &task::priority_, &task::real_start_date_, &task::retake_count_,
          &task::start_date_, &task::difficulty_, &task::task_status_id_, &task::task_type_id_,
          &assignees_table::person_id_
      ),
      join<task>(on(c(&entity::uuid_id_) == c(&task::entity_id_))),
      join<assignees_table>(on(c(&assignees_table::task_id_) == c(&task::uuid_id_))),
      where(
          ((!in_project_id.is_nil() && c(&entity::project_id_) == in_project_id) || in_project_id.is_nil()) &&
          ((!in_entity_type_id.is_nil() && c(&entity::entity_type_id_) == in_entity_type_id) ||
           in_entity_type_id.is_nil())
      )
  );
  std::map<uuid, entities_and_tasks_t> l_entities_and_tasks_map{};
  std::map<uuid, std::size_t> l_task_id_set{};
  for (auto&& [

           uuid_id_, name_, status_, episode_id_, description_, preview_file_id_, canceled_, data_, task_id_,
           estimation_, end_date_, due_date_, done_date_, duration_, last_comment_date_, last_preview_file_id_,
           priority_, real_start_date_, retake_count_, start_date_, difficulty_, task_status_id_, task_type_id_,
           person_id_

  ] : l_rows) {
    if (!l_entities_and_tasks_map.contains(uuid_id_)) {
      l_entities_and_tasks_map.emplace(
          uuid_id_,
          entities_and_tasks_t{
              .uuid_id_         = uuid_id_,
              .name_            = name_,
              .status_          = status_,
              .episode_id_      = episode_id_,
              .description_     = description_,
              .preview_file_id_ = preview_file_id_,
              .canceled_        = canceled_,
              .data_            = data_,
              .frame_in_        = data_.contains("frame_in") ? data_["frame_in"].get<std::int32_t>() : 0,
              .frame_out_       = data_.contains("frame_out") ? data_["frame_out"].get<std::int32_t>() : 0,
              .fps_             = data_.contains("fps") ? data_["fps"].get<std::int32_t>() : 0,
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

DOODLE_GET_BY_PARENT_ID_SQL(assets_file_helper::database_t);
DOODLE_GET_BY_PARENT_ID_SQL(assets_helper::database_t);

DOODLE_UUID_TO_ID(project_helper::database_t)
DOODLE_UUID_TO_ID(user_helper::database_t)
DOODLE_UUID_TO_ID(metadata::kitsu::task_type_t)
DOODLE_UUID_TO_ID(assets_file_helper::database_t)
DOODLE_UUID_TO_ID(assets_helper::database_t)
DOODLE_UUID_TO_ID(computer)
DOODLE_UUID_TO_ID(person)
DOODLE_UUID_TO_ID(attendance_helper::database_t)

DOODLE_ID_TO_UUID(project_helper::database_t)
DOODLE_ID_TO_UUID(user_helper::database_t)
DOODLE_ID_TO_UUID(metadata::kitsu::task_type_t)
DOODLE_ID_TO_UUID(assets_file_helper::database_t)
DOODLE_ID_TO_UUID(assets_helper::database_t)
DOODLE_ID_TO_UUID(computer)
DOODLE_ID_TO_UUID(attendance_helper::database_t)

template <>
work_xlsx_task_info_helper::database_t sqlite_database::get_by_uuid<work_xlsx_task_info_helper::database_t>(
    const uuid& in_uuid
) {
  auto l_list     = impl_->get_by_uuid<work_xlsx_task_info_helper::database_t>(in_uuid);
  l_list.user_id_ = impl_->id_to_uuid<user_helper::database_t>(l_list.user_ref_);
  return l_list;
}
template <>
person sqlite_database::get_by_uuid<person>(const uuid& in_uuid) {
  auto l_p         = impl_->get_by_uuid<person>(in_uuid);
  l_p.departments_ = impl_->storage_any_.select(
      &person_department_link::department_id_,
      sqlite_orm::where(sqlite_orm::c(&person_department_link::person_id_) == in_uuid)
  );
  return l_p;
}

DOODLE_GET_BY_UUID_SQL(user_helper::database_t)
DOODLE_GET_BY_UUID_SQL(metadata::kitsu::task_type_t)
DOODLE_GET_BY_UUID_SQL(assets_file_helper::database_t)
DOODLE_GET_BY_UUID_SQL(assets_helper::database_t)
DOODLE_GET_BY_UUID_SQL(project_helper::database_t)
DOODLE_GET_BY_UUID_SQL(metadata::kitsu::assets_type_t)
DOODLE_GET_BY_UUID_SQL(computer)
DOODLE_GET_BY_UUID_SQL(server_task_info)
DOODLE_GET_BY_UUID_SQL(project_status)
DOODLE_GET_BY_UUID_SQL(task_type)
DOODLE_GET_BY_UUID_SQL(asset_type)
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
DOODLE_GET_ALL_SQL(department)
DOODLE_GET_ALL_SQL(studio)
DOODLE_GET_ALL_SQL(status_automation)
DOODLE_GET_ALL_SQL(organisation)
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
    auto l_list_dep = impl_->storage_any_.select(
        sqlite_orm::columns(&person_department_link::department_id_),
        sqlite_orm::where(sqlite_orm::c(&person_department_link::person_id_) == i.uuid_id_)
    );
    i.departments_ = l_list_dep |
                     ranges::views::transform([](const std::tuple<uuid>& in) { return std::get<uuid>(in); }) |
                     ranges::to_vector;
  }

  return l_list;
}

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
DOODLE_INSTALL_SQL(asset_type)
DOODLE_INSTALL_SQL(task_type_asset_type_link)
DOODLE_INSTALL_SQL(project_task_type_link)
DOODLE_INSTALL_SQL(project_task_status_link)
DOODLE_INSTALL_SQL(department)
DOODLE_INSTALL_SQL(person)
DOODLE_INSTALL_SQL(task)
DOODLE_INSTALL_SQL(project_asset_type_link)
DOODLE_INSTALL_SQL(entity)
DOODLE_INSTALL_SQL(attendance_helper::database_t)

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
DOODLE_INSTALL_RANGE(asset_type)
DOODLE_INSTALL_RANGE(task)

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
DOODLE_REMOVE_BY_UUID(attendance_helper::database_t)

}  // namespace doodle