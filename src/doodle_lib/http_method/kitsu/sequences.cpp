//
// Created by TD on 25-4-28.
//
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/working_file.h"
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/kitsu/kitsu_result.h>

#include <sqlite_orm/sqlite_orm.h>

namespace doodle::http {
namespace {
struct sequences_with_tasks_result {
  sequences_with_tasks_result() = default;

  explicit sequences_with_tasks_result(const entity& in_entity)
      : uuid_id_(in_entity.uuid_id_),
        name_(in_entity.name_),
        status_(in_entity.status_),
        episode_id_(in_entity.parent_id_),
        description_(in_entity.description_),
        preview_file_id_(in_entity.preview_file_id_),
        canceled_(in_entity.canceled_),
        frame_in_(0),
        frame_out_(0),
        fps_(0) {}

  decltype(entity::uuid_id_) uuid_id_;
  decltype(entity::name_) name_;
  decltype(entity::status_) status_;
  decltype(entity::parent_id_) episode_id_;
  decltype(entity::description_) description_;
  decltype(entity::preview_file_id_) preview_file_id_;
  decltype(entity::canceled_) canceled_;

  std::int32_t frame_in_;
  std::int32_t frame_out_;
  std::int32_t fps_;

  struct task_t {
    task_t() = default;
    explicit task_t(const task& in_task, bool in_is_subscribed)
        : uuid_id_(in_task.uuid_id_),
          estimation_(in_task.estimation_),
          entity_id_(in_task.entity_id_),
          end_date_(in_task.end_date_),
          due_date_(in_task.due_date_),
          done_date_(in_task.done_date_),
          duration_(in_task.duration_),
          last_comment_date_(in_task.last_comment_date_),
          last_preview_file_id_(in_task.last_preview_file_id_),
          priority_(in_task.priority_),
          real_start_date_(in_task.real_start_date_),
          retake_count_(in_task.retake_count_),
          start_date_(in_task.start_date_),
          difficulty_(in_task.difficulty_),
          task_status_id_(in_task.task_status_id_),
          task_type_id_(in_task.task_type_id_),
          is_subscribed_(in_is_subscribed) {}
    decltype(task::uuid_id_) uuid_id_;
    decltype(task::estimation_) estimation_;
    decltype(entity::uuid_id_) entity_id_;
    decltype(task::end_date_) end_date_;
    decltype(task::due_date_) due_date_;
    decltype(task::done_date_) done_date_;
    decltype(task::duration_) duration_;
    decltype(task::last_comment_date_) last_comment_date_;
    decltype(task::last_preview_file_id_) last_preview_file_id_;
    decltype(task::priority_) priority_;
    decltype(task::real_start_date_) real_start_date_;
    decltype(task::retake_count_) retake_count_;
    decltype(task::start_date_) start_date_;
    decltype(task::difficulty_) difficulty_;
    decltype(task::task_status_id_) task_status_id_;
    decltype(task::task_type_id_) task_type_id_;
    std::vector<uuid> assigners_;
    bool is_subscribed_;
    friend void to_json(nlohmann::json& j, const task_t& p) {
      j["id"]                   = p.uuid_id_;
      j["estimation"]           = p.estimation_;
      j["entity_id"]            = p.entity_id_;
      j["end_date"]             = p.end_date_;
      j["due_date"]             = p.due_date_;
      j["done_date"]            = p.done_date_;
      j["duration"]             = p.duration_;
      j["is_subscribed"]        = p.is_subscribed_;
      j["last_comment_date"]    = p.last_comment_date_;
      j["last_preview_file_id"] = p.last_preview_file_id_;
      j["priority"]             = p.priority_;
      j["real_start_date"]      = p.real_start_date_;
      j["retake_count"]         = p.retake_count_;
      j["start_date"]           = p.start_date_;
      j["difficulty"]           = p.difficulty_;
      j["task_type_id"]         = p.task_type_id_;
      j["task_status_id"]       = p.task_status_id_;
      j["assignees"]            = p.assigners_;
      j["working_files"]        = nlohmann::json::array();
    }
  };
  std::vector<task_t> tasks_;
  // to json
  friend void to_json(nlohmann::json& j, const sequences_with_tasks_result& p) {
    j["id"]              = p.uuid_id_;
    j["name"]            = p.name_;
    j["status"]          = p.status_;
    j["episode_id"]      = p.episode_id_;
    j["description"]     = p.description_;

    j["frame_in"]        = p.frame_in_ ? nlohmann::json{} = p.frame_in_ : nlohmann::json::object();
    j["frame_out"]       = p.frame_out_ ? nlohmann::json{} = p.frame_out_ : nlohmann::json::object();
    j["fps"]             = p.fps_ ? nlohmann::json{} = p.fps_ : nlohmann::json::object();
    j["preview_file_id"] = p.preview_file_id_;
    j["canceled"]        = p.canceled_;
    j["tasks"]           = p.tasks_;
  }
};

auto get_get_entities_and_tasks(const person& in_person, const uuid& in_project_id, const uuid& in_entity_type_id) {
  DOODLE_CHICK(!in_entity_type_id.is_nil(), "实体类型id不可为空");

  std::vector<sequences_with_tasks_result> l_ret{};
  auto l_sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_outsource_select = select(
      &outsource_studio_authorization::entity_id_,
      where(c(&outsource_studio_authorization::studio_id_) == in_person.studio_id_)
  );

  auto l_subscriptions_for_user = l_sql.get_person_subscriptions(in_person, in_project_id, in_entity_type_id);

  auto l_rows                   = l_sql.impl_->storage_any_.select(
      columns(object<entity>(true), object<task>(true), &assignees_table::person_id_), from<entity>(),
      left_outer_join<task>(on(c(&entity::uuid_id_) == c(&task::entity_id_))),
      left_outer_join<assignees_table>(on(c(&assignees_table::task_id_) == c(&task::uuid_id_))),
      where(
          c(&entity::entity_type_id_) == in_entity_type_id &&
          ((in_project_id.is_nil() || c(&entity::project_id_) == in_project_id) &&
           (in_person.role_ != person_role_type::outsource || in(&entity::uuid_id_, l_outsource_select)))

      )
  );
  std::map<uuid, sequences_with_tasks_result> l_entities_and_tasks_map{};
  std::map<uuid, std::size_t> l_task_id_set{};
  for (auto&& [l_entity, l_task, l_person_id] : l_rows) {
    if (!l_entities_and_tasks_map.contains(l_entity.uuid_id_)) {
      l_entities_and_tasks_map.emplace(l_entity.uuid_id_, sequences_with_tasks_result{l_entity});
    }
    if (!l_task.uuid_id_.is_nil()) {
      if (!l_task_id_set.contains(l_task.uuid_id_)) {
        l_entities_and_tasks_map[l_entity.uuid_id_].tasks_.emplace_back(
            sequences_with_tasks_result::task_t{
                l_task,
                l_subscriptions_for_user.contains(l_task.uuid_id_),
            }
        );
        l_task_id_set.emplace(l_task.uuid_id_, l_entities_and_tasks_map[l_entity.uuid_id_].tasks_.size() - 1);
      }
      if (!l_person_id.is_nil())
        l_entities_and_tasks_map[l_entity.uuid_id_].tasks_[l_task_id_set.at(l_task.uuid_id_)].assigners_.emplace_back(
            l_person_id
        );
    }
  }
  l_ret = l_entities_and_tasks_map | ranges::views::values | ranges::to_vector;
  return l_ret;
}

}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> sequences_with_tasks::get(session_data_ptr in_handle) {
  auto& l_sql    = g_ctx().get<sqlite_database>();
  auto l_type_id = l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_sequence});

  uuid l_project_uuid{};
  for (auto&& [key, value, has] : in_handle->url_.params())
    if (key == "project_id" && has) l_project_uuid = from_uuid_str(value);

  auto l_r = get_get_entities_and_tasks(person_.person_, l_project_uuid, l_type_id.uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_r);
}

namespace {
struct data_project_sequences_args {
  uuid episode_id_{};
  std::string name_{};
  std::string description_{};
  // form json
  friend void from_json(const nlohmann::json& j, data_project_sequences_args& p) {
    if (j.contains("episode_id") && !j.at("episode_id").is_null()) j.at("episode_id").get_to(p.episode_id_);
    if (j.contains("description") && !j.at("description").is_null()) j.at("description").get_to(p.description_);
    j.at("name").get_to(p.name_);
  }
};
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> data_project_sequences::post(session_data_ptr in_handle) {
  person_.check_in_project(id_);
  person_.check_not_outsourcer();
  auto& l_sql = g_ctx().get<sqlite_database>();
  auto l_args = in_handle->get_json().get<data_project_sequences_args>();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始在项目 {} 创建/获取序列 name {} episode_id {}",
      person_.person_.email_, person_.person_.get_full_name(), id_, l_args.name_, l_args.episode_id_
  );

  using namespace sqlite_orm;
  auto l_sq_list    = l_sql.impl_->storage_any_.get_all<entity>(where(
      c(&entity::entity_type_id_) ==
          l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_sequence}).uuid_id_ &&
      c(&entity::parent_id_) == l_args.episode_id_ && c(&entity::project_id_) == id_ &&
      c(&entity::name_) == l_args.name_
  ));
  auto l_entity_ptr = std::make_shared<entity>();
  if (l_sq_list.empty()) {
    l_entity_ptr->name_        = l_args.name_;
    l_entity_ptr->description_ = l_args.description_;
    l_entity_ptr->parent_id_   = l_args.episode_id_;
    l_entity_ptr->project_id_  = id_;
    l_entity_ptr->entity_type_id_ =
        l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_sequence}).uuid_id_;
    l_entity_ptr->created_by_ = person_.person_.uuid_id_;
    co_await l_sql.install(l_entity_ptr);
    socket_io::broadcast("sequence:new", nlohmann::json{{"sequence_id", l_entity_ptr->uuid_id_}, {"project_id", id_}});
  } else
    *l_entity_ptr = l_sq_list.front();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成在项目 {} 创建/获取序列 sequence_id {} name {}",
      person_.person_.email_, person_.person_.get_full_name(), id_, l_entity_ptr->uuid_id_, l_entity_ptr->name_
  );

  co_return in_handle->make_msg(nlohmann::json{} = *l_entity_ptr);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_sequences::get(session_data_ptr in_handle) {
  person_.check_in_project(id_);
  auto& l_sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_sq_list = l_sql.impl_->storage_any_.get_all<entity>(where(
      c(&entity::entity_type_id_) ==
          l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_sequence}).uuid_id_ &&
      c(&entity::project_id_) == id_
  ));
  co_return in_handle->make_msg(nlohmann::json{} = l_sq_list);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_sequence_instance::get(session_data_ptr in_handle) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_data = l_sql.get_by_uuid<entity>(id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_data);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_sequence_instance::delete_(
    session_data_ptr in_handle
) {
  auto l_sql      = g_ctx().get<sqlite_database>();
  auto l_sequence = std::make_shared<entity>(l_sql.get_by_uuid<entity>(id_));
  if (!(l_sequence->created_by_ == person_.person_.uuid_id_ &&
        l_sql.is_person_in_project(person_.person_.uuid_id_, l_sequence->project_id_)))
    person_.check_project_manager(l_sequence->project_id_);
  bool l_force{};
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 序列 {} ", person_.person_.email_, person_.person_.get_full_name(),
      l_sequence->name_
  );
  for (auto&& [key, value, has] : in_handle->url_.params())
    if (key == "force" && has) l_force = true;
  if (!l_force) {
    l_sequence->canceled_ = true;
    co_await l_sql.update(l_sequence);
    socket_io::broadcast(
        "sequence:update",
        nlohmann::json{{"sequence_id", l_sequence->uuid_id_}, {"project_id", l_sequence->project_id_}}
    );
  } else {
    auto l_task     = l_sql.get_tasks_for_entity(l_sequence->uuid_id_);
    auto l_task_ids = l_task | ranges::views::transform([](const task& in) { return in.uuid_id_; }) | ranges::to_vector;
    co_await l_sql.remove<task>(l_task_ids);
    co_await l_sql.remove<entity>(l_sequence->uuid_id_);
    socket_io::broadcast(
        "sequence:delete",
        nlohmann::json{{"sequence_id", l_sequence->uuid_id_}, {"project_id", l_sequence->project_id_}}
    );
  }
  co_return in_handle->make_msg_204();
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_task_types_create_tasks::post(
    session_data_ptr in_handle
) {
  person_.check_in_project(project_id_);
  person_.check_not_outsourcer();
  auto l_sql          = g_ctx().get<sqlite_database>();
  auto l_task_type    = l_sql.get_by_uuid<task_type>(task_type_id_);
  auto l_type_name    = std::string{magic_enum::enum_name(entity_type_)};
  l_type_name.front() = std::toupper(l_type_name.front());
  auto l_entity_type  = l_sql.get_entity_type_by_name(l_type_name);
  std::vector<entity> l_entities{};

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始在项目 {} 批量创建任务 task_type_id {} entity_type {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, task_type_id_, l_type_name
  );

  if (auto l_json = in_handle->get_json(); l_json.is_array()) {
    l_entities.reserve(l_json.size());
    for (auto&& l_id : l_json.get<std::vector<uuid>>())
      if (auto l_ent = l_sql.get_by_uuid<entity>(l_id); l_ent.project_id_ == project_id_)
        l_entities.emplace_back(std::move(l_ent));
  } else {
    auto l_episode_id = l_json.value("episode_id", uuid{});
    using namespace sqlite_orm;
    l_entities = l_sql.impl_->storage_any_.get_all<entity>(where(
        c(&entity::project_id_) == project_id_ && c(&entity::entity_type_id_) == l_entity_type.uuid_id_ &&
        (l_episode_id.is_nil() || c(&entity::parent_id_) == l_episode_id)
    ));
  }
  std::shared_ptr<std::vector<task>> l_tasks = std::make_shared<std::vector<task>>();
  auto l_task_status = l_sql.get_task_status_by_name(std::string{doodle_config::task_status_todo});

  std::vector<actions_projects_task_types_create_tasks_result> l_result{};
  for (auto&& l_enit : l_entities) {
    using namespace sqlite_orm;
    if (l_sql.impl_->storage_any_.count<task>(
            where(c(&task::entity_id_) == l_enit.uuid_id_) && c(&task::task_type_id_) == task_type_id_
        ))
      continue;  // 已经存在任务
    auto& l_task = l_tasks->emplace_back(
        task{
            .name_           = "main",
            .created_at_     = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()},
            .updated_at_     = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()},
            .project_id_     = project_id_,
            .task_type_id_   = task_type_id_,
            .task_status_id_ = l_task_status.uuid_id_,
            .entity_id_      = l_enit.uuid_id_,
            .assigner_id_    = person_.person_.uuid_id_,
        }
    );
    l_result.emplace_back(actions_projects_task_types_create_tasks_result{l_task, l_task_type, l_task_status});
  }
  if (!l_tasks->empty()) co_await l_sql.install_range(l_tasks);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成在项目 {} 批量创建任务 task_type_id {} 数量 {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, task_type_id_, l_tasks->size()
  );

  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

}  // namespace doodle::http
