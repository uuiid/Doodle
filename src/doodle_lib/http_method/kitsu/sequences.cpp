//
// Created by TD on 25-4-28.
//
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

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
  std::vector<sequences_with_tasks_result> l_ret{};
  auto l_sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_subscriptions_for_user = l_sql.get_person_subscriptions(in_person, in_project_id, in_entity_type_id);
  auto l_rows                   = l_sql.impl_->storage_any_.select(
      columns(object<entity>(true), object<task>(true), &assignees_table::person_id_),
      join<task>(on(c(&entity::uuid_id_) == c(&task::entity_id_))),
      left_outer_join<assignees_table>(on(c(&assignees_table::task_id_) == c(&task::uuid_id_))),
      where(
          (in_project_id.is_nil() || c(&entity::project_id_) == in_project_id) &&
          (in_entity_type_id.is_nil() || c(&entity::entity_type_id_) == in_entity_type_id)
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
    if (j.contains("episode_id")) j.at("episode_id").get_to(p.episode_id_);
    j.at("name").get_to(p.name_);
    j.at("description").get_to(p.description_);
  }
};
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> data_project_sequences::post(session_data_ptr in_handle) {
  person_.check_project_access(id_);
  auto& l_sql = g_ctx().get<sqlite_database>();
  auto l_args = in_handle->get_json().get<data_project_sequences_args>();
  using namespace sqlite_orm;
  auto l_sq_list    = l_sql.impl_->storage_any_.get_all<entity>(where(
      c(&entity::entity_type_id_) ==
          l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_sequence}).uuid_id_ &&
      c(&entity::parent_id_) == l_args.episode_id_ && c(&entity::project_id_) == id_ &&
      c(&entity::name_) == l_args.name_
  ));
  auto l_entity_ptr = std::make_shared<entity>();
  if (l_sq_list.empty()) {
    l_entity_ptr->uuid_id_     = core_set::get_set().get_uuid();
    l_entity_ptr->name_        = l_args.name_;
    l_entity_ptr->description_ = l_args.description_;
    l_entity_ptr->parent_id_   = l_args.episode_id_;
    l_entity_ptr->project_id_  = id_;
    l_entity_ptr->entity_type_id_ =
        l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_sequence}).uuid_id_;
    l_entity_ptr->created_by_ = person_.person_.uuid_id_;
    co_await l_sql.install(l_entity_ptr);
    socket_io::broadcast(
        "episode:new", nlohmann::json{{"sequence_id", l_entity_ptr->uuid_id_}, {"project_id", id_}}, "/events"
    );
  } else
    *l_entity_ptr = l_sq_list.front();

  co_return in_handle->make_msg(nlohmann::json{} = *l_entity_ptr);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_sequences::get(session_data_ptr in_handle) {
  person_.check_project_access(id_);
  auto& l_sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_sq_list = l_sql.impl_->storage_any_.get_all<entity>(where(
      c(&entity::entity_type_id_) ==
          l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_sequence}).uuid_id_ &&
      c(&entity::project_id_) == id_
  ));
  co_return in_handle->make_msg(nlohmann::json{} = l_sq_list);
}

}  // namespace doodle::http
