//
// Created by TD on 24-8-20.
//

#include "doodle_core/metadata/task.h"

#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/notification.h"
#include "doodle_core/sqlite_orm/detail/sqlite_database_impl.h"
#include <doodle_core/metadata/project.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <range/v3/view/unique.hpp>
#include <sqlite_orm/sqlite_orm.h>
#include <tuple>
#include <vector>

namespace doodle::http {

namespace {
template <typename Where, typename OrderBy>
auto get_todo_fun(Where&& in_where, OrderBy&& in_order_by) {
  auto l_sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_task = l_sql.impl_->storage_any_.select(
      columns(
          object<task>(true), &project::name_, &project::has_avatar_, object<entity>(true), &asset_type::name_,
          &task_type::name_, &task_type::for_entity_, &task_type::color_, &task_status::name_, &task_status::color_,
          &task_status::short_name_,
          object<entity_asset_extend>(true)
      ),  //
      join<project>(on(c(&task::project_id_) == c(&project::uuid_id_))),
      join<task_type>(on(c(&task::task_type_id_) == c(&task_type::uuid_id_))),
      join<task_status>(on(c(&task::task_status_id_) == c(&task_status::uuid_id_))),
      join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
      join<assignees_table>(on(c(&task::uuid_id_) == c(&assignees_table::task_id_))),
      join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
      left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
      where(in_where), in_order_by
  );
  std::vector<todo_t> l_ret;
  l_ret.reserve(l_task.size());
  for (auto&& l_tub : l_task) {
    l_ret.emplace_back(std::make_from_tuple<todo_t>(l_tub));
  }

  auto l_task_ids = l_ret | ranges::views::transform([](const todo_t& in) { return in.uuid_id_; }) | ranges::to_vector;

  {
    std::map<uuid, const comment*> l_map_comm;
    auto l_comms = l_sql.impl_->storage_any_.get_all<comment>(
        sqlite_orm::where(sqlite_orm::in(&comment::object_id_, l_task_ids)), sqlite_orm::order_by(&comment::created_at_)
    );
    for (auto&& i : l_comms) {
      if (!l_map_comm.contains(i.object_id_)) l_map_comm[i.object_id_] = &i;
    }

    for (auto& i : l_ret) {
      if (l_map_comm.contains(i.uuid_id_)) {
        auto&& l_c = l_map_comm.at(i.uuid_id_);
        i.last_comment_ =
            todo_t::comment_t{.text_ = l_c->text_, .date_ = l_c->created_at_, .person_id_ = l_c->person_id_};
      }
    }
  }
  std::map<uuid, todo_t*> l_map_task;
  for (auto&& i : l_ret) l_map_task[i.uuid_id_] = &i;

  for (auto l_ass = l_sql.impl_->storage_any_.get_all<assignees_table>(
           sqlite_orm::where(sqlite_orm::in(&assignees_table::task_id_, l_task_ids))
       );
       auto&& i : l_ass) {
    if (l_map_task.contains(i.task_id_)) l_map_task.at(i.task_id_)->assignees_.emplace_back(i.person_id_);
  }
  return l_ret;
}
}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> data_task_status_links::post(session_data_ptr in_handle) {
  person_.check_manager();
  auto l_sql              = g_ctx().get<sqlite_database>();
  auto l_json             = in_handle->get_json();

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
  auto l_sql  = g_ctx().get<sqlite_database>();
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
  auto l_sql                                 = g_ctx().get<sqlite_database>();
  auto l_person_data                         = l_sql.get_by_uuid<person>(id_);
  auto l_task_ids                            = in_handle->get_json()["task_ids"].get<std::vector<uuid>>();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始批量分配任务 to_person_id {} task_count {}", person_.person_.email_,
      person_.person_.get_full_name(), l_person_data.uuid_id_, l_task_ids.size()
  );
  std::shared_ptr<std::vector<task>> l_tasks = std::make_shared<std::vector<task>>();
  std::shared_ptr<std::vector<assignees_table>> l_assignees_table = std::make_shared<std::vector<assignees_table>>();
  std::shared_ptr<std::vector<notification>> l_notifications      = std::make_shared<std::vector<notification>>();
  l_tasks->reserve(l_task_ids.size());
  l_assignees_table->reserve(l_task_ids.size());
  l_notifications->reserve(l_task_ids.size());
  using namespace sqlite_orm;
  auto l_tasks_get = l_sql.impl_->storage_any_.get_all<task>(where(in(&task::uuid_id_, l_task_ids)));
  for (auto&& l_task : l_tasks_get) {
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
      person_.person_.email_, person_.person_.get_full_name(), l_person_data.uuid_id_, l_task_ids.size(), l_tasks->size(),
      l_notifications->size()
  );
  co_return in_handle->make_msg(nlohmann::json{} = *l_tasks);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_user_tasks::get(session_data_ptr in_handle) {
  auto& sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_prjs    = sql.get_person_projects(person_.person_);
  auto l_pej_ids = l_prjs | ranges::views::transform([](const project& in) { return in.uuid_id_; }) | ranges::to_vector;

  auto l_todo    = get_todo_fun(
      in(&task::project_id_, l_pej_ids) && c(&assignees_table::person_id_) == person_.person_.uuid_id_ &&
          c(&task_status::is_done_) == false,
      order_by(&entity::name_)
  );

  co_return in_handle->make_msg(nlohmann::json{} = l_todo);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_user_done_tasks::get(session_data_ptr in_handle) {
  auto& sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_prjs    = sql.get_person_projects(person_.person_);
  auto l_pej_ids = l_prjs | ranges::views::transform([](const project& in) { return in.uuid_id_; }) | ranges::to_vector;

  auto l_todo    = get_todo_fun(
      in(&task::project_id_, l_pej_ids) && c(&assignees_table::person_id_) == person_.person_.uuid_id_ &&
          c(&task_status::is_done_) == true,
      multi_order_by(order_by(&task::end_date_).desc(), order_by(&task_type::name_), order_by(&entity::name_))
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_todo);
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
  auto& sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_prjs    = sql.get_person_projects(person_.person_);
  auto l_pej_ids = l_prjs | ranges::views::transform([](const project& in) { return in.uuid_id_; }) | ranges::to_vector;

  auto l_todo    = get_todo_fun(
      c(&task_status::is_feedback_request_) && in(&task::project_id_, l_pej_ids) &&
          ((person_.person_.role_ == person_role_type::supervisor &&
            in(&task_type::department_id_, person_.person_.departments_)) ||
           person_.person_.role_ != person_role_type::supervisor),
      order_by(&entity::name_)
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_todo);
}
boost::asio::awaitable<boost::beast::http::message_generator> tasks_comments::get(session_data_ptr in_handle) {
  auto& sql = g_ctx().get<sqlite_database>();
  nlohmann::json l_r{};
  l_r = sql.get_comments(id_);
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
    auto l_sql = g_ctx().get<sqlite_database>();
    using namespace sqlite_orm;
    constexpr auto sequence = "sequence"_alias.for_<entity>();
    constexpr auto episode  = "episode"_alias.for_<entity>();
    for (auto &&[task, l_entity_asset_extend,project_name, project_has_avatar,

      entity_uuid_id, entity_name, entity_description, entity_preview_file_id, entity_canceled,
      entity_project_id, entity_source_id,

      sequence_name,

      episode_uuid_id, episode_name,

      asset_type_name,

      task_type_name, task_type_for_entity, task_type_color,

      task_status_name, task_status_color, task_status_short_name
    ] : l_sql.impl_->storage_any_.select(columns(

             object<task>(true), object<entity_asset_extend>(true), &project::name_, &project::has_avatar_,

             &entity::uuid_id_, &entity::name_, &entity::description_, &entity::preview_file_id_, &entity::canceled_,
             &entity::project_id_, &entity::source_id_,

             sequence->*&entity::name_,

             episode->*&entity::uuid_id_, episode->*&entity::name_,

             &asset_type::name_,

             &task_type::name_, &task_type::for_entity_, &task_type::color_,

             &task_status::name_, &task_status::color_, &task_status::short_name_

         ),
         from<task>(),
        join<task_type>(on(c(&task_type::uuid_id_) == c(&task::task_type_id_))),
        join<task_status>(on(c(&task_status::uuid_id_) == c(&task::task_status_id_))),
        join<entity>(on(c(&entity::uuid_id_) == c(&task::entity_id_))),
        join<asset_type>(on(c(&asset_type::uuid_id_) == c(&entity::entity_type_id_))),
        join<project>(on(c(&project::uuid_id_) == c(&task::project_id_))),
        join<project_status>(on(c(&project_status::uuid_id_) == c(&project::project_status_id_))),
        left_outer_join<sequence>(on(c(sequence->*&entity::uuid_id_) == c(&entity::parent_id_))),
        left_outer_join<episode>(on(c(episode->*&entity::uuid_id_) == c(sequence->*&entity::parent_id_))),
        left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
        where(
        (
        (c(&task::start_date_) >= start_time_ && c(&task::start_date_) <= end_time_) ||
        (c(&task::end_date_) >= start_time_ && c(&task::end_date_) <= end_time_)
        ) &&
          in(&task::uuid_id_,
            select(&assignees_table::task_id_, from<assignees_table>(), where(c(&assignees_table::person_id_) == person_id_)))
        ),
        multi_order_by(order_by(&project::name_), order_by(episode->*&entity::name_), order_by(sequence->*&entity::name_),
          order_by(&asset_type::name_),      order_by(&task_type::name_))

         )) {
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
      l_item.task_.assignees_ = l_sql.impl_->storage_any_.select(
          &assignees_table::person_id_, from<assignees_table>(),
          where(c(&assignees_table::task_id_) == l_item.task_.uuid_id_)
      );
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
  auto l_task = g_ctx().get<sqlite_database>().get_by_uuid<task>(id_);
  person_.check_delete_access(l_task.project_id_);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除任务 {} ", person_.person_.email_, person_.person_.get_full_name(),
      l_task.uuid_id_
  );
  co_await g_ctx().get<sqlite_database>().remove<task>(id_);
  co_return in_handle->make_msg_204();
}
boost::asio::awaitable<boost::beast::http::message_generator> data_tasks_full::get(session_data_ptr in_handle) {
  auto l_sql         = g_ctx().get<sqlite_database>();

  auto l_task        = l_sql.get_by_uuid<task>(id_);
  auto l_task_type   = l_sql.get_by_uuid<task_type>(l_task.task_type_id_);
  auto l_project     = l_sql.get_by_uuid<project>(l_task.project_id_);
  auto l_task_status = l_sql.get_by_uuid<task_status>(l_task.task_status_id_);
  auto l_entity      = l_sql.get_by_uuid<entity>(l_task.entity_id_);
  auto l_asset_type  = l_sql.get_by_uuid<asset_type>(l_entity.entity_type_id_);
  using namespace sqlite_orm;
  auto l_is_subscribed =
      l_sql.impl_->storage_any_.count(
          &subscription::uuid_id_, from<subscription>(),
          where(c(&subscription::task_id_) == id_ && c(&subscription::person_id_) == person_.person_.uuid_id_)
      ) > 0;
  auto l_assignees = l_sql.impl_->storage_any_.select(
      object<person>(true), from<person>(), where(in(&person::uuid_id_, l_task.assignees_))
  );
  auto l_working_files = l_sql.get_working_file_by_task(id_);
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
          {"working_files", l_working_files},
      }
  );
  co_return in_handle->make_msg(l_ret);
}

}  // namespace doodle::http
