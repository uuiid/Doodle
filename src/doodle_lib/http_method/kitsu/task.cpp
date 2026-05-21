//
// Created by TD on 24-8-20.
//

#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/notification.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/subscription.h>
#include <doodle_core/metadata/task.h>

#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>

#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <range/v3/view/unique.hpp>
#include <sqlite_orm/sqlite_orm.h>
#include <tuple>
#include <vector>

namespace doodle::http {

namespace {
// 获取用户的待办事项后处理
auto get_todo_post_process(std::vector<todo_t>& in_todos) {
  auto& l_sql = get_sqlite_database();
  using namespace orm;
  auto l_task_ids =
      in_todos | ranges::views::transform([](const todo_t& in) { return in.uuid_id_; }) | ranges::to_vector;

  {
    std::map<uuid, const comment*> l_map_comm;
    for (auto&& i : select(l_sql)
                        .columns(object<comment>())
                        .from<comment>()
                        .where(c(&comment::object_id_).in(l_task_ids))
                        .order_by (&comment::created_at_)()) {
      if (!l_map_comm.contains(i.object_id_)) l_map_comm[i.object_id_] = &i;
    }

    for (auto& i : in_todos) {
      if (l_map_comm.contains(i.uuid_id_)) {
        auto&& l_c = l_map_comm.at(i.uuid_id_);
        i.last_comment_ =
            todo_t::comment_t{.text_ = l_c->text_, .date_ = l_c->created_at_, .person_id_ = l_c->person_id_};
      }
    }
  }
  std::map<uuid, todo_t*> l_map_task;
  for (auto&& i : in_todos) l_map_task[i.uuid_id_] = &i;

  for (auto l_ass = select(l_sql)
                        .columns(object<assignees_table>())
                        .from<assignees_table>()
                        .where(c(&assignees_table::task_id_).in(l_task_ids))();
       auto&& i : l_ass) {
    if (l_map_task.contains(i.task_id_)) l_map_task.at(i.task_id_)->assignees_.emplace_back(i.person_id_);
  }
  return in_todos;
}

auto get_todo_fun() {
  using namespace orm;
  return select(get_sqlite_database())
      .columns(
          object<task>(), &project::name_, &project::has_avatar_, object<entity>(), &asset_type::name_,
          &task_type::name_, &task_type::for_entity_, &task_type::color_, &task_status::name_, &task_status::color_,
          &task_status::short_name_, object<entity_asset_extend>()
      )
      .join<project>(&task::project_id_, &project::uuid_id_)
      .join<task_type>(&task::task_type_id_, &task_type::uuid_id_)
      .join<task_status>(&task::task_status_id_, &task_status::uuid_id_)
      .join<entity>(&task::entity_id_, &entity::uuid_id_)
      .join<assignees_table>(&task::uuid_id_, &assignees_table::task_id_)
      .join<asset_type>(&entity::entity_type_id_, &asset_type::uuid_id_)
      .left_outer_join<entity_asset_extend>(&entity_asset_extend::entity_id_, &entity::uuid_id_);
}
}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> data_task_status_links::post(session_data_ptr in_handle) {
  person_.check_manager();
  auto& l_sql = get_sqlite_database();
  auto l_json = in_handle->get_json();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始设置任务状态关联 project_id {} task_status_id {}",
      person_.person_.email_, person_.person_.get_full_name(), l_json["project_id"].get<uuid>(),
      l_json["task_status_id"].get<uuid>()
  );

  auto l_task_status_link = std::make_shared<project_task_status_link>(
      l_sql.get_project_task_status_link(l_json["project_id"].get<uuid>(), l_json["task_status_id"].get<uuid>())
          .value_or(project_task_status_link{})
  );
  l_json.get_to(*l_task_status_link);
  l_task_status_link->id_ == 0 ? co_await l_sql.install(l_task_status_link) : co_await l_sql.update(l_task_status_link);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成设置任务状态关联 id {} project_id {} task_status_id {}",
      person_.person_.email_, person_.person_.get_full_name(), l_task_status_link->id_, l_task_status_link->project_id_,
      l_task_status_link->task_status_id_
  );

  co_return in_handle->make_msg(nlohmann::json{} = *l_task_status_link);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_tasks::put(session_data_ptr in_handle) {
  auto& l_sql = get_sqlite_database();
  auto l_task = std::make_shared<task>(l_sql.get_by_uuid<task>(id_));
  person_.check_task_action_access(*l_task);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始更新任务 task_id {} project_id {}", person_.person_.email_,
      person_.person_.get_full_name(), l_task->uuid_id_, l_task->project_id_
  );
  in_handle->get_json().get_to(*l_task);
  co_await l_sql.update(l_task);
  // l_task->assigner_id_ = l_person->person_.uuid_id_;

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成更新任务 task_id {} project_id {}", person_.person_.email_,
      person_.person_.get_full_name(), l_task->uuid_id_, l_task->project_id_
  );
  co_return in_handle->make_msg(nlohmann::json{} = *l_task);
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_persons_assign::put(session_data_ptr in_handle) {
  auto& l_sql        = get_sqlite_database();
  auto l_person_data = l_sql.get_by_uuid<person>(id_);
  auto l_task_ids    = in_handle->get_json()["task_ids"].get<std::vector<uuid>>();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始批量分配任务 to_person_id {} task_count {}", person_.person_.email_,
      person_.person_.get_full_name(), l_person_data.uuid_id_, l_task_ids.size()
  );
  std::shared_ptr<std::vector<task>> l_tasks                      = std::make_shared<std::vector<task>>();
  std::shared_ptr<std::vector<assignees_table>> l_assignees_table = std::make_shared<std::vector<assignees_table>>();
  std::shared_ptr<std::vector<notification>> l_notifications      = std::make_shared<std::vector<notification>>();
  l_tasks->reserve(l_task_ids.size());
  l_assignees_table->reserve(l_task_ids.size());
  l_notifications->reserve(l_task_ids.size());
  using namespace orm;

  for (auto&& l_task : select(l_sql).columns(object<task>()).from<task>().where(c(&task::uuid_id_).in(l_task_ids))()) {
    person_.check_task_department_access(l_task, person_.person_);
    // 这里需要检查一下, 是否已经将任务分配给了这个人
    if (l_sql.is_task_assigned_to_person(l_task.uuid_id_, l_person_data.uuid_id_)) continue;
    assignees_table l_task_assign{};
    l_task.assigner_id_ = person_.person_.uuid_id_;
    l_tasks->emplace_back(l_task);
    l_task_assign.person_id_ = l_person_data.uuid_id_;
    l_task_assign.task_id_   = l_task.uuid_id_;
    l_assignees_table->emplace_back(l_task_assign);
    // 这里需要检查一下, 任务的分配人是否是当前用户
    if (person_.person_.uuid_id_ != l_person_data.uuid_id_) {
      notification l_notification{};
      l_notification.type_      = notification_type::assignation;
      l_notification.task_id_   = l_task.uuid_id_;
      l_notification.author_id_ = person_.person_.uuid_id_;
      l_notification.person_id_ = l_person_data.uuid_id_;
      l_notifications->emplace_back(l_notification);
    }
  }
  co_await l_sql.update_range(l_tasks);
  co_await l_sql.install_range(l_assignees_table);
  co_await l_sql.install_range(l_notifications);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 完成批量分配任务 to_person_id {} requested_task_count {} assigned_task_count {} notify_count {}",
      person_.person_.email_, person_.person_.get_full_name(), l_person_data.uuid_id_, l_task_ids.size(),
      l_tasks->size(), l_notifications->size()
  );
  co_return in_handle->make_msg(nlohmann::json{} = *l_tasks);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_user_tasks::get(session_data_ptr in_handle) {
  auto& sql = get_sqlite_database();
  using namespace orm;
  auto l_prjs    = sql.get_person_projects(person_.person_);
  auto l_pej_ids = l_prjs | ranges::views::transform([](const project& in) { return in.uuid_id_; }) | ranges::to_vector;

  auto l_todo =
      get_todo_fun()
          .where(
              c(&task::project_id_).in(l_pej_ids) && c(&assignees_table::person_id_) == person_.person_.uuid_id_ &&
              c(&task_status::is_done_) == false
          )
          .order_by(&entity::name_);
  auto l_ret = l_todo().to_vector<todo_t>();
  co_return in_handle->make_msg(nlohmann::json{} = get_todo_post_process(l_ret));
}

boost::asio::awaitable<boost::beast::http::message_generator> data_user_done_tasks::get(session_data_ptr in_handle) {
  auto& sql = get_sqlite_database();
  using namespace orm;
  auto l_prjs    = sql.get_person_projects(person_.person_);
  auto l_pej_ids = l_prjs | ranges::views::transform([](const project& in) { return in.uuid_id_; }) | ranges::to_vector;

  auto l_todo    = get_todo_fun()
                    .where(
                        c(&task::project_id_).in(l_pej_ids) &&
                        c(&assignees_table::person_id_) == person_.person_.uuid_id_ && c(&task_status::is_done_) == true
                    )
                    .order_by(&task::end_date_, false)
                    .order_by(&task_type::name_)
                    .order_by(&entity::name_);
  auto l_ret = l_todo().to_vector<todo_t>();
  co_return in_handle->make_msg(nlohmann::json{} = get_todo_post_process(l_ret));
}
boost::asio::awaitable<boost::beast::http::message_generator> tasks_to_check::get(session_data_ptr in_handle) {
  switch (person_.person_.role_) {
    case person_role_type::admin:
    case person_role_type::supervisor:
    case person_role_type::manager:
      break;

    case person_role_type::user:
    case person_role_type::client:
    case person_role_type::vendor:
      co_return in_handle->make_msg("[]"s);
      break;
  }
  auto& sql = get_sqlite_database();
  using namespace orm;
  auto l_prjs    = sql.get_person_projects(person_.person_);
  auto l_pej_ids = l_prjs | ranges::views::transform([](const project& in) { return in.uuid_id_; }) | ranges::to_vector;

  auto l_todo    = get_todo_fun();
  auto l_base_where = c(&task::project_id_).in(l_pej_ids) && c(&task_status::is_feedback_request_);
  if (person_.is_supervisor()) {
    l_todo.where(l_base_where && c(&task_type::department_id_).in(person_.person_.departments_));
  } else {
    l_todo.where(l_base_where);
  }
  l_todo.order_by(&entity::name_);
  auto l_ret = l_todo().to_vector<todo_t>();
  co_return in_handle->make_msg(nlohmann::json{} = get_todo_post_process(l_ret));
}
namespace {
auto get_comments(const uuid& in_task_id) {
  using namespace doodle::orm;
  auto& l_sql = get_sqlite_database();
  // 查询所有评论，按创建时间倒序
  auto l_comm = select(l_sql)
                    .columns(object<comment>())
                    .from<comment>()
                    .where(c(&comment::object_id_) == in_task_id)
                    .order_by(&comment::created_at_, false)()
                    .to_vector<comment>();

  // 收集相关 id
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

  // 查询相关实体
  auto l_persons = select(l_sql)
                       .columns(object<person>())
                       .from<person>()
                       .where(c(&person::uuid_id_).in(l_person_ids))()
                       .to_vector<person>();
  auto l_task_statuses = select(l_sql)
                             .columns(object<task_status>())
                             .from<task_status>()
                             .where(c(&task_status::uuid_id_).in(l_task_status_ids))()
                             .to_vector<task_status>();
  auto l_editors = select(l_sql)
                       .columns(object<person>())
                       .from<person>()
                       .where(c(&person::uuid_id_).in(l_editor_ids))()
                       .to_vector<person>();
  auto l_acknowledgements = select(l_sql)
                                .columns(object<comment_acknoledgments>())
                                .from<comment_acknoledgments>()
                                .where(c(&comment_acknoledgments::comment_id_).in(l_comment_ids))()
                                .to_vector<comment_acknoledgments>();

  // 预览文件与评论关联
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
  // 查询预览文件与评论的关联
  auto l_previews =
      select(l_sql)
          .columns(
              &preview_file::uuid_id_, &preview_file::task_id_, &preview_file::revision_, &preview_file::extension_,
              &preview_file::width_, &preview_file::height_, &preview_file::duration_, &preview_file::status_,
              &preview_file::validation_status_, &preview_file::original_name_, &preview_file::position_,
              &preview_file::annotations_, &comment_preview_link::comment_id_
          )
          .from<preview_file>()
          .join<comment_preview_link>(&preview_file::uuid_id_, &comment_preview_link::preview_file_id_)
          .where(c(&comment_preview_link::comment_id_).in(l_comment_ids))()
          .to_vector<previews_and_comment_id_t>();

  auto l_mention = select(l_sql)
                       .columns(object<comment_mentions>())
                       .from<comment_mentions>()
                       .where(c(&comment_mentions::comment_id_).in(l_comment_ids))()
                       .to_vector<comment_mentions>();
  auto l_department_mention = select(l_sql)
                                  .columns(object<comment_department_mentions>())
                                  .from<comment_department_mentions>()
                                  .where(c(&comment_department_mentions::comment_id_).in(l_comment_ids))()
                                  .to_vector<comment_department_mentions>();
  auto l_attachment_file = select(l_sql)
                               .columns(object<attachment_file>())
                               .from<attachment_file>()
                               .where(c(&attachment_file::comment_id_).in(l_comment_ids))()
                               .to_vector<attachment_file>();

  // 构建映射表
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
}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> tasks_comments::get(session_data_ptr in_handle) {
  nlohmann::json l_r{};
  l_r = get_comments(id_);
  co_return in_handle->make_msg(l_r);
}
namespace {

struct open_tasks_get_t {
  task task_;
  entity_asset_extend entity_asset_extend_;
  decltype(project::name_) project_name_;
  decltype(project::uuid_id_) project_id_;
  decltype(project::has_avatar_) project_has_avatar_;
  decltype(entity::uuid_id_) entity_id_;
  decltype(entity::name_) entity_name_;
  decltype(entity::description_) entity_description_;
  decltype(entity::preview_file_id_) entity_preview_file_id_;
  decltype(entity::parent_id_) entity_source_id_;
  decltype(asset_type::name_) entity_type_name_;
  decltype(entity::canceled_) entity_canceled_;
  decltype(entity::name_) sequence_name_;
  decltype(entity::uuid_id_) episode_id_;
  decltype(entity::name_) episode_name_;
  decltype(task::estimation_) estimation_;
  decltype(task::duration_) duration_;
  decltype(task::start_date_) start_date_;
  decltype(task::due_date_) due_date_;
  decltype(task::done_date_) done_date_;
  decltype(task_type::name_) type_name_;
  decltype(task_type::for_entity_) task_type_for_entity_;
  decltype(task_status::name_) status_name_;
  decltype(task_type::color_) type_color_;
  decltype(task_status::color_) status_color_;
  decltype(task_status::short_name_) status_short_name_;
  // to json
  friend void to_json(nlohmann::json& in_j, const open_tasks_get_t& in_p) {
    to_json(in_j, in_p.entity_asset_extend_);  // 先调用额外数据写入, 将写入id属性
    to_json(in_j, in_p.task_);                 // 后调用 task 数据写入, 将id属性覆盖为 task.id

    in_j["project_name"]           = in_p.project_name_;
    in_j["project_id"]             = in_p.project_id_;
    in_j["project_has_avatar"]     = in_p.project_has_avatar_;
    in_j["entity_id"]              = in_p.entity_id_;
    in_j["entity_name"]            = in_p.entity_name_;
    in_j["entity_description"]     = in_p.entity_description_;
    in_j["entity_preview_file_id"] = in_p.entity_preview_file_id_;
    in_j["entity_source_id"]       = in_p.entity_source_id_;
    in_j["entity_type_name"]       = in_p.entity_type_name_;
    in_j["entity_canceled"]        = in_p.entity_canceled_;
    in_j["sequence_name"]          = in_p.sequence_name_;
    in_j["episode_id"]             = in_p.episode_id_;
    in_j["episode_name"]           = in_p.episode_name_;
    in_j["estimation"]             = in_p.estimation_;
    in_j["duration"]               = in_p.duration_;
    in_j["start_date"]             = in_p.start_date_;
    in_j["due_date"]               = in_p.due_date_;
    in_j["done_date"]              = in_p.done_date_;
    in_j["type_name"]              = in_p.type_name_;
    in_j["task_type_for_entity"]   = in_p.task_type_for_entity_;
    in_j["status_name"]            = in_p.status_name_;
    in_j["type_color"]             = in_p.type_color_;
    in_j["status_color"]           = in_p.status_color_;
    in_j["status_short_name"]      = in_p.status_short_name_;
  }
};
struct data_tasks_open_tasks_get_args {
  uuid person_id_{};
  chrono::system_zoned_time start_time_{};
  chrono::system_zoned_time end_time_{};

  void parse_args(const boost::urls::params_ref& in_params) {
    for (auto&& [key, value, has] : in_params) {
      if (key == "person_id" && has) person_id_ = from_uuid_str(value);
      if (key == "start_date" && has) start_time_ = from_chrono_time_zone_str(value);
      if (key == "end_date" && has) end_time_ = from_chrono_time_zone_str(value);
    }
    if (person_id_.is_nil() || start_time_ == chrono::system_zoned_time{} || end_time_ == chrono::system_zoned_time{})
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "缺失查询参数"});
    default_logger_raw()->debug(
        "person_id:{}, start_date:{}, end_date:{}", person_id_, start_time_.get_sys_time(), end_time_.get_sys_time()
    );
  }

  std::vector<open_tasks_get_t> get() {
    std::vector<open_tasks_get_t> l_ret{};
    auto& l_sql = get_sqlite_database();
    using namespace orm;
    auto sequence = alias<entity>("sequence");
    auto episode  = alias<entity>("episode");
    for (auto&& [task, l_entity_asset_extend, project_name, project_has_avatar,

                entity_uuid_id, entity_name, entity_description, entity_preview_file_id, entity_canceled,
                entity_source_id,

                sequence_name,

                episode_uuid_id, episode_name,

                asset_type_name,

                task_type_name, task_type_for_entity, task_type_color,

                task_status_name, task_status_color, task_status_short_name] :
         select(l_sql)
             .columns(

                 object<task>(), object<entity_asset_extend>(), &project::name_, &project::has_avatar_,

                 &entity::uuid_id_, &entity::name_, &entity::description_, &entity::preview_file_id_,
                 &entity::canceled_, &entity::source_id_,

                 sequence->*&entity::name_,

                 episode->*&entity::uuid_id_, episode->*&entity::name_,

                 &asset_type::name_,

                 &task_type::name_, &task_type::for_entity_, &task_type::color_,

                 &task_status::name_, &task_status::color_, &task_status::short_name_

             )
             .from<task>()
             .join<task_type>(&task::task_type_id_, &task_type::uuid_id_)
             .join<task_status>(&task::task_status_id_, &task_status::uuid_id_)
             .join<entity>(&task::entity_id_, &entity::uuid_id_)
             .join<asset_type>(&entity::entity_type_id_, &asset_type::uuid_id_)
             .join<project>(&task::project_id_, &project::uuid_id_)
             .join<project_status>(&project::project_status_id_, &project_status::uuid_id_)
             .join<assignees_table>(&task::uuid_id_, &assignees_table::task_id_)
             .left_outer_join(sequence, sequence->*&entity::uuid_id_, &entity::parent_id_)
             .left_outer_join(episode, episode->*&entity::uuid_id_, sequence->*&entity::parent_id_)
             .left_outer_join<entity_asset_extend>(&entity::uuid_id_, &entity_asset_extend::entity_id_)
             .where(
                 ((c(&task::start_date_) >= start_time_ && c(&task::start_date_) <= end_time_) ||
                  (c(&task::end_date_) >= start_time_ && c(&task::end_date_) <= end_time_)) &&
                 c(&assignees_table::person_id_) == person_id_
             )
             .order_by(&project::name_)
             .order_by(&asset_type::name_)
             .order_by(&task_type::name_)()) {
      l_ret.emplace_back(
          open_tasks_get_t{
              .task_                   = task,
              .entity_asset_extend_    = l_entity_asset_extend,
              .project_name_           = project_name,
              .project_id_             = task.project_id_,
              .project_has_avatar_     = project_has_avatar,
              .entity_id_              = entity_uuid_id,
              .entity_name_            = entity_name,
              .entity_description_     = entity_description,
              .entity_preview_file_id_ = entity_preview_file_id,
              .entity_source_id_       = entity_source_id,
              .entity_type_name_       = asset_type_name,
              .entity_canceled_        = entity_canceled,
              .sequence_name_          = sequence_name,
              .episode_id_             = episode_uuid_id,
              .episode_name_           = episode_name,
              .estimation_             = task.estimation_,
              .duration_               = task.duration_,
              .start_date_             = task.start_date_,
              .due_date_               = task.due_date_,
              .done_date_              = task.done_date_,
              .type_name_              = task_type_name,
              .task_type_for_entity_   = task_type_for_entity,
              .status_name_            = task_status_name,
              .type_color_             = task_type_color,
              .status_color_           = task_status_color,
              .status_short_name_      = task_status_short_name
          }
      );
    }
    for (auto l_item : l_ret) {
      l_item.task_.assignees_ = select(l_sql)
                                    .columns(&assignees_table::person_id_)
                                    .from<assignees_table>()
                                    .where(c(&assignees_table::task_id_) == l_item.task_.uuid_id_)()
                                    .to_vector();
    }
    return l_ret;
  }
};

}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> data_tasks_open_tasks::get(session_data_ptr in_handle) {
  data_tasks_open_tasks_get_args l_args{};
  l_args.parse_args(in_handle->url_.params());

  co_return in_handle->make_msg(nlohmann::json{} = l_args.get());
}
boost::asio::awaitable<boost::beast::http::message_generator> data_tasks::delete_(session_data_ptr in_handle) {
  auto l_task = get_sqlite_database().get_by_uuid<task>(id_);
  person_.check_delete_access(l_task.project_id_);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除任务 {} ", person_.person_.email_, person_.person_.get_full_name(),
      l_task.uuid_id_
  );
  co_await get_sqlite_database().remove<task>(id_);
  co_return in_handle->make_msg_204();
}
boost::asio::awaitable<boost::beast::http::message_generator> data_tasks_full::get(session_data_ptr in_handle) {
  auto& l_sql        = get_sqlite_database();

  auto l_task        = l_sql.get_by_uuid<task>(id_);
  auto l_task_type   = l_sql.get_by_uuid<task_type>(l_task.task_type_id_);
  auto l_project     = l_sql.get_by_uuid<project>(l_task.project_id_);
  auto l_task_status = l_sql.get_by_uuid<task_status>(l_task.task_status_id_);
  auto l_entity      = l_sql.get_by_uuid<entity>(l_task.entity_id_);
  auto l_asset_type  = l_sql.get_by_uuid<asset_type>(l_entity.entity_type_id_);
  using namespace doodle::orm;
  // 判断是否订阅
  auto l_is_subscribed =
      doodle::orm::select(l_sql)
          .columns(count(&subscription::uuid_id_))
          .from<subscription>()
          .where(c(&subscription::task_id_) == id_ && c(&subscription::person_id_) == person_.person_.uuid_id_)()
          .to_single() > 0;

  // 查询 assignees
  auto l_assignees = doodle::orm::select(l_sql)
                         .columns(object<person>())
                         .from<person>()
                         .where(c(&person::uuid_id_).in(l_task.assignees_))()
                         .to_vector<person>();
  nlohmann::json l_ret{};
  l_ret = l_task;
  l_ret.update(
      nlohmann::json{
          {"entity", l_entity},
          {"entity_type", l_asset_type},
          {"is_subscribed", l_is_subscribed},
          {"persons", l_assignees},
          {"project", l_project},
          {"task_type", l_task_type},
          {"task_status", l_task_status},
      }
  );
  co_return in_handle->make_msg(l_ret);
}

}  // namespace doodle::http
