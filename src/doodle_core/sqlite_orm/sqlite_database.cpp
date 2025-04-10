//
// Created by TD on 24-9-12.
//

#include "sqlite_database.h"

#include <doodle_core/core/app_base.h>
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
std::vector<entity_task_t> sqlite_database::get_assets_and_tasks(
    const uuid& in_project, const person& in_current_user
) {
  auto l_temporal_type_ids = get_temporal_type_ids();
  std::map<uuid, std::shared_ptr<asset_type>> l_asset_types;  // <uuid, std::shared_ptr<asset_type>>
  {
    auto l_ass_types = impl_->storage_any_.get_all<asset_type>(
        sqlite_orm::where(sqlite_orm::not_in(&asset_type::uuid_id_, l_temporal_type_ids))
    );
    l_asset_types = l_ass_types | ranges::views::transform([](const asset_type& in) {
                      return std::make_pair(in.uuid_id_, std::make_shared<asset_type>(in));
                    }) |
                    ranges::to<std::map<uuid, std::shared_ptr<asset_type>>>();
  }
  std::vector<entity_task_t> l_result;
  auto l_entt_list = in_project.is_nil()
                         ? impl_->storage_any_.get_all<entity>(
                               sqlite_orm::where(sqlite_orm::not_in(&entity::entity_type_id_, l_temporal_type_ids)),
                               sqlite_orm::order_by(&entity::name_)
                           )
                         : impl_->storage_any_.get_all<entity>(
                               sqlite_orm::where(
                                   sqlite_orm::not_in(&entity::entity_type_id_, l_temporal_type_ids) &&
                                   sqlite_orm::c(&entity::project_id_) == in_project
                               ),
                               sqlite_orm::order_by(&entity::name_)
                           );
  auto l_entt_id_list =
      l_entt_list | ranges::views::transform([](const entity& in) { return in.uuid_id_; }) | ranges::to_vector;
  auto l_task = impl_->storage_any_.get_all<task>(sqlite_orm::where(sqlite_orm::in(&task::entity_id_, l_entt_id_list)));
  std::set<uuid> l_current_user_subscribed_tasks;
  {
    std::map<uuid, uuid> l_task_person_ids{};
    auto l_task_ids = l_task | ranges::views::transform([](const task& in) { return in.uuid_id_; }) | ranges::to_vector;
    auto l_r        = impl_->storage_any_.get_all<assignees_table>(
        sqlite_orm::where(sqlite_orm::in(&assignees_table::task_id_, l_task_ids))
    );
    auto l_subscription             = impl_->storage_any_.get_all<subscription>(sqlite_orm::where(
        sqlite_orm::in(&subscription::task_id_, l_task_ids) &&
        sqlite_orm::c(&subscription::person_id_) == in_current_user.uuid_id_
    ));
    l_current_user_subscribed_tasks = l_subscription |
                                      ranges::views::transform([](const subscription& in) { return in.task_id_; }) |
                                      ranges::to<std::set<uuid>>();
    l_task_person_ids =
        l_r |
        ranges::views::transform([](const assignees_table& in) { return std::make_pair(in.task_id_, in.person_id_); }) |
        ranges::to<std::map<uuid, uuid>>();

    for (auto&& l_t : l_task) {
      if (l_task_person_ids.contains(l_t.uuid_id_)) l_t.assignees_.emplace_back(l_task_person_ids.at(l_t.uuid_id_));
    }
  }
  l_result.reserve(l_entt_list.size());
  for (auto&& l_entt : l_entt_list) l_result.emplace_back(l_entt);
  std::map<uuid, entity_task_t*> l_map =
      l_result | ranges::views::transform([](entity_task_t& in) { return std::make_pair(in.uuid_id_, &in); }) |
      ranges::to<std::map<uuid, entity_task_t*>>();
  for (auto&& l_t : l_task)
    l_map[l_t.entity_id_]->tasks_.emplace_back(l_t).is_subscribed_ =
        l_current_user_subscribed_tasks.contains(l_t.uuid_id_);
  for (auto&& l_entt : l_result) {
    if (l_asset_types.contains(l_entt.entity_type_id_)) l_entt.asset_type_ = l_asset_types.at(l_entt.entity_type_id_);
  }

  return l_result;
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
std::vector<project_and_status_t> sqlite_database::get_project_and_status(const std::shared_ptr<person>& in_user) {
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
  auto l_r = in_user ? impl_->storage_any_.select(
                           sql_orm_project_and_status_t,
                           join<project_status>(on(c(&project::project_status_id_) == c(&project_status::uuid_id_))),
                           join<project_person_link>(on(c(&project_person_link::project_id_) == c(&project::uuid_id_))),
                           where(c(&project_person_link::person_id_) == in_user->uuid_id_)
                       )
                     : impl_->storage_any_.select(
                           sql_orm_project_and_status_t,
                           join<project_status>(on(c(&project::project_status_id_) == c(&project_status::uuid_id_)))
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
      l_comment.persons_.emplace_back(
          get_comments_t::person_t{
              .uuid_id_    = l_c.person_id_,
              .first_name_ = l_map_person[l_c.person_id_]->first_name_,
              .last_name_  = l_map_person[l_c.person_id_]->last_name_,
              .has_avatar_ = l_map_person[l_c.person_id_]->has_avatar_,
          }
      );
    if (l_map_task_status.contains(l_c.task_status_id_))
      l_comment.task_statuses_.emplace_back(
          get_comments_t::task_status_t{
              .uuid_id_    = l_c.task_status_id_,
              .name_       = l_map_task_status[l_c.task_status_id_]->name_,
              .color_      = l_map_task_status[l_c.task_status_id_]->color_,
              .short_name_ = l_map_task_status[l_c.task_status_id_]->short_name_,
          }
      );
    if (l_map_editor.contains(l_c.editor_id_))
      l_comment.editors_.emplace_back(
          get_comments_t::person_t{
              .uuid_id_    = l_c.editor_id_,
              .first_name_ = l_map_editor[l_c.editor_id_]->first_name_,
              .last_name_  = l_map_editor[l_c.editor_id_]->last_name_,
              .has_avatar_ = l_map_editor[l_c.editor_id_]->has_avatar_,
          }
      );
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
  auto l_p    = impl_->get_by_uuid<person>(in_uuid);
  auto l_list = impl_->storage_any_.select(
      sqlite_orm::columns(&person_department_link::department_id_),
      sqlite_orm::where(sqlite_orm::c(&person_department_link::person_id_) == in_uuid)
  );
  l_p.departments_ = l_list | ranges::views::transform([](const std::tuple<uuid>& in) { return std::get<uuid>(in); }) |
                     ranges::to_vector;
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
DOODLE_INSTALL_SQL(department)
DOODLE_INSTALL_SQL(person)
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