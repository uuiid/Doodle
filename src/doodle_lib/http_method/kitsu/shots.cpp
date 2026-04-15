//
// Created by TD on 25-7-28.
//

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/entity.h"
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/working_file.h>

#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/kitsu/kitsu_result.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/url/url.hpp>

#include <map>
#include <range/v3/view/unique.hpp>
#include <spdlog/spdlog.h>
#include <sqlite_orm/sqlite_orm.h>
#include <vector>

namespace doodle::http {
namespace {
struct shots_with_tasks_result {
  shots_with_tasks_result() = default;

  explicit shots_with_tasks_result(
      const entity& in_shot, const uuid& in_episode_id, const std::string& in_episode_name, const uuid& in_project_id,
      const std::string& in_project_name, const uuid& in_sequence_id, const std::string& in_sequence_name,
      const entity_shot_extend& in_shot_extend
  )
      : uuid_id_(in_shot.uuid_id_),
        description_(in_shot.description_),
        name_(in_shot.name_),
        episode_id_(in_episode_id),
        episode_name_(in_episode_name),
        canceled_(in_shot.canceled_),
        nb_frames_(in_shot.nb_frames_),
        parent_id_(in_shot.parent_id_),
        preview_file_id_(in_shot.preview_file_id_),
        project_id_(in_project_id),
        project_name_(in_project_name),
        sequence_id_(in_sequence_id),
        sequence_name_(in_sequence_name),
        source_id_(in_shot.source_id_),
        nb_entities_out_(in_shot.nb_entities_out_),
        is_casting_standby_(in_shot.is_casting_standby_),
        entity_type_id_(in_shot.entity_type_id_),
        frame_in_(in_shot_extend.frame_in_),
        frame_out_(in_shot_extend.frame_out_),
        fps_(0) {}

  decltype(entity::uuid_id_) uuid_id_;
  decltype(entity::description_) description_;
  decltype(entity::name_) name_;
  decltype(entity::parent_id_) episode_id_;
  decltype(entity::name_) episode_name_;
  decltype(entity::canceled_) canceled_;
  decltype(entity::nb_frames_) nb_frames_;
  decltype(entity::parent_id_) parent_id_;
  decltype(entity::preview_file_id_) preview_file_id_;
  decltype(project::uuid_id_) project_id_;
  decltype(project::name_) project_name_;
  decltype(entity::uuid_id_) sequence_id_;
  decltype(entity::name_) sequence_name_;
  decltype(entity::source_id_) source_id_;
  decltype(entity::nb_entities_out_) nb_entities_out_;
  decltype(entity::is_casting_standby_) is_casting_standby_;
  decltype(entity::entity_type_id_) entity_type_id_;

  decltype(entity_shot_extend::frame_in_) frame_in_;
  decltype(entity_shot_extend::frame_out_) frame_out_;
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
          nb_assets_ready_(in_task.nb_assets_ready_),
          nb_drawings_(in_task.nb_drawings_),
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
    decltype(task::nb_assets_ready_) nb_assets_ready_;
    decltype(task::nb_drawings_) nb_drawings_;
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
      j["nb_assets_ready"]      = p.nb_assets_ready_;
      j["nb_drawings"]          = p.nb_drawings_;
      j["working_files"]        = nlohmann::json::array();
    }
  };
  std::vector<task_t> tasks_;
  // to json
  friend void to_json(nlohmann::json& j, const shots_with_tasks_result& p) {
    j["id"]                 = p.uuid_id_;
    j["description"]        = p.description_;
    j["name"]               = p.name_;
    j["frame_in"]           = p.frame_in_;
    j["frame_out"]          = p.frame_out_;
    j["fps"]                = p.fps_;
    j["episode_id"]         = p.episode_id_;
    j["episode_name"]       = p.episode_name_;
    j["canceled"]           = p.canceled_;
    j["nb_frames"]          = p.nb_frames_;
    j["parent_id"]          = p.parent_id_;
    j["preview_file_id"]    = p.preview_file_id_;
    j["project_id"]         = p.project_id_;
    j["project_name"]       = p.project_name_;
    j["sequence_id"]        = p.sequence_id_;
    j["sequence_name"]      = p.sequence_name_;
    j["source_id"]          = p.source_id_;
    j["nb_entities_out"]    = p.nb_entities_out_;
    j["is_casting_standby"] = p.is_casting_standby_;
    j["entity_type_id"]     = p.entity_type_id_;
    j["tasks"]              = p.tasks_;
    auto&& l_data           = j["data"];
    l_data["fps"];
    l_data["frame_in"];
    l_data["frame_out"];
    l_data["resolution"];
    l_data["max_retakes"];
  }
};

struct make_shots_with_tasks_result_t {
  person person_;
  uuid entity_type_id_;
  uuid project_id_;
  std::vector<uuid> episode_id_;
  std::vector<uuid> sequence_id_;
  std::int32_t offset_{};
  std::int32_t limit_{300};

  static constexpr auto get_sequence_table() {
    using namespace sqlite_orm;
    return "sequence"_alias.for_<entity>();
  }
  static constexpr auto get_episode_table() {
    using namespace sqlite_orm;
    return "episode"_alias.for_<entity>();
  }

  template <typename T>
  auto get_shots_with_tasks(T&& in_dynamic_query) {
    std::vector<shots_with_tasks_result> l_ret{};
    std::map<uuid, std::size_t> l_shots_ids{};
    std::set<uuid> l_tasks_ids;
    auto l_sql = get_sqlite_database();
    using namespace sqlite_orm;
    auto l_subscriptions_for_user = l_sql.get_person_subscriptions(person_, project_id_, entity_type_id_);

    auto l_outsource_select       = select(
        &outsource_studio_authorization::entity_id_,
        where(c(&outsource_studio_authorization::studio_id_) == person_.studio_id_)
    );
    constexpr auto sequence = get_sequence_table();
    constexpr auto episode  = get_episode_table();

    auto l_list             = l_sql.impl_->storage_any_.select(
        columns(
            object<entity>(true), object<task>(true), episode->*&entity::uuid_id_, episode->*&entity::name_,
            sequence->*&entity::uuid_id_, sequence->*&entity::name_, &assignees_table::person_id_, &project::uuid_id_,
            &project::name_, object<entity_shot_extend>(true)
        ),
        from<entity>(), join<project>(on(c(&entity::project_id_) == c(&project::uuid_id_))),
        left_outer_join<entity_shot_extend>(on(c(&entity_shot_extend::entity_id_) == c(&entity::uuid_id_))),
        join<sequence>(on(c(&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
        join<episode>(on(c(sequence->*&entity::uuid_id_) == c(episode->*&entity::uuid_id_))),
        left_outer_join<task>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
        left_outer_join<assignees_table>(on(c(&assignees_table::task_id_) == c(&task::uuid_id_))),
        where(std::forward<T>(in_dynamic_query)),
        multi_order_by(
            order_by(episode->*&entity::name_), order_by(sequence->*&entity::name_), order_by(&entity::name_)
        ),
        limit(offset_, limit_)
    );
    for (auto&& [

             l_entity, l_task, l_episode_id, l_episode_name, l_sequence_id, l_sequence_name,

             l_assignee_id, l_project_id, l_project_name, l_shot_extend

    ] : l_list) {
      if (!l_shots_ids.contains(l_entity.uuid_id_)) {
        l_ret.emplace_back(
            shots_with_tasks_result{
                l_entity, l_episode_id, l_episode_name, l_project_id, l_project_name, l_sequence_id, l_sequence_name,
                l_shot_extend
            }
        );
        l_shots_ids.emplace(l_entity.uuid_id_, l_ret.size() - 1);
      }
      if (!l_task.uuid_id_.is_nil()) {
        auto&& l_r = l_ret[l_shots_ids[l_entity.uuid_id_]].tasks_.emplace_back(
            shots_with_tasks_result::task_t{l_task, l_subscriptions_for_user.contains(l_task.uuid_id_)}
        );
        if (!l_assignee_id.is_nil()) l_r.assigners_.emplace_back(l_assignee_id);
      }
    }

    return l_ret;
  }

  explicit make_shots_with_tasks_result_t(
      const person& in_person, const uuid& in_entity_type_id, const boost::urls::url& in_url
  )
      : person_(in_person), entity_type_id_(in_entity_type_id) {
    for (auto&& [key, value, has] : in_url.params()) {
      if (key == "project_id" && has) project_id_ = from_uuid_str(value);
      if (key == "episode_id" && has) episode_id_.emplace_back(from_uuid_str(value));
      if (key == "sequence_id" && has) sequence_id_.emplace_back(from_uuid_str(value));
      if (key == "offset" && has) offset_ = std::stoi(value);
      if (key == "limit" && has) limit_ = std::stoi(value);
    }
  }
  auto operator()() {
    using namespace sqlite_orm;
    constexpr auto sequence = get_sequence_table();
    constexpr auto episode  = get_episode_table();
    auto l_dynamic_query    = dynamic_where(get_sqlite_database().impl_->storage_any_);
    l_dynamic_query.push_back(c(&entity::entity_type_id_) == entity_type_id_);
    if (person_.role_ == person_role_type::outsource) {
      auto l_outsource_select = select(
          &outsource_studio_authorization::entity_id_,
          where(c(&outsource_studio_authorization::studio_id_) == person_.studio_id_)
      );
      l_dynamic_query.push_back(
          in(&entity::uuid_id_, l_outsource_select) || in(sequence->*&entity::uuid_id_, l_outsource_select) ||
          in(episode->*&entity::uuid_id_, l_outsource_select)
      );
    }

    if (!project_id_.is_nil()) l_dynamic_query.push_back(c(&entity::project_id_) == project_id_);
    if (!episode_id_.empty()) l_dynamic_query.push_back(in(episode->*&entity::uuid_id_, episode_id_));
    if (!sequence_id_.empty()) l_dynamic_query.push_back(in(sequence->*&entity::uuid_id_, sequence_id_));
    return get_shots_with_tasks(std::move(l_dynamic_query));
  }
};

}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> data_shots_with_tasks::get(session_data_ptr in_handle) {
  auto& l_sql    = get_sqlite_database();
  auto l_type_id = l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_shot});

  make_shots_with_tasks_result_t l_make_result(person_.person_, l_type_id.uuid_id_, in_handle->url_);

  co_return in_handle->make_msg(nlohmann::json{} = l_make_result());
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_shots::get(session_data_ptr in_handle) {
  co_return in_handle->make_msg_204();
}
namespace {
struct data_project_shots_args {
  std::string name_{};
  uuid sequence_id_{};
  nlohmann::json data_{};
  std::int32_t nb_frames_{};
  std::string description_{};
  // from json
  friend void from_json(const nlohmann::json& j, data_project_shots_args& p) {
    j.at("name").get_to(p.name_);
    if (j.contains("sequence_id")) j.at("sequence_id").get_to(p.sequence_id_);
    if (j.contains("data")) p.data_ = j.at("data");
    if (j.contains("nb_frames")) j.at("nb_frames").get_to(p.nb_frames_);
    if (j.contains("description")) j.at("description").get_to(p.description_);
  }
};
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> data_project_shots::post(session_data_ptr in_handle) {
  auto l_args = in_handle->get_json().get<data_project_shots_args>();
  auto l_sql  = get_sqlite_database();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始在项目 {} 创建/获取镜头 name {} sequence_id {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, l_args.name_, l_args.sequence_id_
  );

  using namespace sqlite_orm;
  if (!l_args.sequence_id_.is_nil() &&
      l_sql.impl_->storage_any_.count<entity>(where(
          c(&entity::uuid_id_) == l_args.sequence_id_ &&
          c(&entity::entity_type_id_) ==
              l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_sequence}).uuid_id_
      )) == 0)
    throw_exception(
        http_request_error{boost::beast::http::status::bad_request, "未知的序列 id " + to_string(l_args.sequence_id_)}
    );

  if (auto l_list = l_sql.impl_->storage_any_.get_all<entity>(where(
          c(&entity::name_) == l_args.name_ && c(&entity::parent_id_) == l_args.sequence_id_ &&
          c(&entity::parent_id_) == project_id_ &&
          c(&entity::entity_type_id_) ==
              l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_sequence}).uuid_id_
      ));
      !l_list.empty())
    co_return in_handle->make_msg(nlohmann::json{} = l_list.front());

  auto l_shot = std::make_shared<entity>(entity{
      .name_           = l_args.name_,
      .description_    = l_args.description_,
      .nb_frames_      = l_args.nb_frames_,
      .project_id_     = project_id_,
      .entity_type_id_ = l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_shot}).uuid_id_,
      .parent_id_      = l_args.sequence_id_,
      .created_by_     = person_.person_.uuid_id_
  });
  co_await l_sql.install(l_shot);
  auto l_shot_extend =
      std::make_shared<entity_shot_extend>(entity_shot_extend{.entity_id_ = l_shot->uuid_id_, .frame_in_ = 1001});
  co_await l_sql.install(l_shot_extend);
  socket_io::broadcast(
      socket_io::shot_new_broadcast_t{
          .shot_id_ = l_shot->uuid_id_, .episode_id_ = l_args.sequence_id_, .project_id_ = project_id_
      }
  );
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成在项目 {} 创建镜头 shot_id {} name {} sequence_id {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, l_shot->uuid_id_, l_shot->name_,
      l_args.sequence_id_
  );

  nlohmann::json l_result;
  l_result = *l_shot;
  l_result.update(*l_shot_extend);
  co_return in_handle->make_msg(l_result);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_shot::get(session_data_ptr in_handle) {
  auto l_sql = get_sqlite_database();
  auto l_ent = l_sql.get_by_uuid<entity>(id_);
  auto l_ext = l_sql.get_entity_shot_extend(id_);
  nlohmann::json l_result;
  l_result = l_ent;
  l_result.update(l_ext);

  co_return in_handle->make_msg(l_result);
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_task_types_shots_create_tasks::post(
    session_data_ptr in_handle
) {
  auto l_sql       = get_sqlite_database();
  auto l_task_type = l_sql.get_by_uuid<task_type>(task_type_id_);
  std::vector<entity> l_shots;

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始在项目 {} 批量创建镜头任务 task_type_id {}", person_.person_.email_,
      person_.person_.get_full_name(), project_id_, task_type_id_
  );

  if (auto l_json = in_handle->get_json(); l_json.is_array()) {
    for (auto&& i : l_json.get<std::vector<uuid>>())
      if (auto l_shot = l_sql.get_by_uuid<entity>(i); l_shot.project_id_ == project_id_)
        l_shots.emplace_back(std::move(l_shot));
  } else {
    using namespace sqlite_orm;
    uuid l_id{};
    for (auto&& [key, value, has] : in_handle->url_.params())
      if (key == "id" && has) l_id = from_uuid_str(value);
    l_shots = l_sql.impl_->storage_any_.get_all<entity>(where(
        c(&entity::project_id_) == project_id_ &&
        c(&entity::entity_type_id_) ==
            l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_shot}).uuid_id_ &&
        (l_id.is_nil() || c(&entity::uuid_id_) == l_id)
    ));
  }
  std::shared_ptr<std::vector<task>> l_tasks = std::make_shared<std::vector<task>>();
  std::vector<actions_projects_task_types_create_tasks_result> l_results{};
  auto l_task_status = l_sql.get_task_status_by_name(std::string{doodle_config::task_status_todo});
  for (auto&& l_enit : l_shots) {
    using namespace sqlite_orm;
    if (l_sql.impl_->storage_any_.count<task>(
            where(c(&task::entity_id_) == l_enit.uuid_id_) && c(&task::task_type_id_) == task_type_id_
        ))
      continue;  // 已经存在任务
    auto& l_task = l_tasks->emplace_back(
        task{
            .name_           = "main",
            .project_id_     = project_id_,
            .task_type_id_   = task_type_id_,
            .task_status_id_ = l_task_status.uuid_id_,
            .entity_id_      = l_enit.uuid_id_,
            .assigner_id_    = person_.person_.uuid_id_,
        }
    );
  }
  if (!l_tasks->empty()) co_await l_sql.install_range(l_tasks);

  for (auto&& l_task : *l_tasks)
    l_results.emplace_back(actions_projects_task_types_create_tasks_result{l_task, l_task_type, l_task_status});

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成在项目 {} 批量创建镜头任务 task_type_id {} 数量 {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, task_type_id_, l_tasks->size()
  );

  co_return in_handle->make_msg(nlohmann::json{} = l_results);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_shot::delete_(session_data_ptr in_handle) {
  auto l_sql  = get_sqlite_database();
  auto l_shot = std::make_shared<entity>(l_sql.get_by_uuid<entity>(id_));
  if (!(
          (l_shot->created_by_ == person_.person_.uuid_id_ &&
           l_sql.is_person_in_project(person_.person_.uuid_id_, l_shot->project_id_)) ||
          l_sql.is_entity_outsourced(id_, person_.person_.studio_id_, l_shot->parent_id_)

      ))
    person_.check_project_manager(l_shot->project_id_);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 {}({}) 镜头", person_.person_.email_,
      person_.person_.get_full_name(), l_shot->name_, l_shot->uuid_id_
  );

  bool l_force{};
  for (auto&& [key, value, has] : in_handle->url_.params())
    if (key == "force" && has) l_force = true;
  l_force = l_force || person_.is_outsourcer();  // 这里比较特殊, 外包商可以直接删除, 不需要先标记为取消
  if (!l_force) {
    l_shot->canceled_ = true;
    co_await l_sql.update(l_shot);
    socket_io::broadcast(
        socket_io::shot_update_broadcast_t{.shot_id_ = l_shot->uuid_id_, .project_id_ = l_shot->project_id_}
    );
  } else {
    auto l_task     = l_sql.get_tasks_for_entity(l_shot->uuid_id_);
    auto l_task_ids = l_task | ranges::views::transform([](const task& in) { return in.uuid_id_; }) | ranges::to_vector;
    co_await l_sql.remove<task>(l_task_ids);
    co_await l_sql.remove<entity>(l_shot->uuid_id_);

    socket_io::broadcast(
        socket_io::shot_delete_broadcast_t{.shot_id_ = l_shot->uuid_id_, .project_id_ = l_shot->project_id_}
    );
  }
  co_return in_handle->make_msg_204();
}
struct actions_projects_shots_import_frame_range_args {
  struct shot_frame_range_t {
    std::string name_;
    std::int32_t frame_in_;
    std::int32_t frame_out_;
    // from json
    friend void from_json(const nlohmann::json& j, shot_frame_range_t& p) {
      j.at("name").get_to(p.name_);
      j.at("frame_in").get_to(p.frame_in_);
      j.at("frame_out").get_to(p.frame_out_);
    }
  };
  std::vector<shot_frame_range_t> shots_;
  // from json
  friend void from_json(const nlohmann::json& j, actions_projects_shots_import_frame_range_args& p) {
    j.at("shots").get_to(p.shots_);
  }
};
struct name_all_t : sqlite_orm::alias_tag {
  static const std::string& get() {
    static const std::string res = "name_all";
    return res;
  }
};
namespace {}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_projects_shots_import_frame_range, post) {
  auto l_args = in_handle->get_json().get<actions_projects_shots_import_frame_range_args>();
  auto l_sql  = get_sqlite_database();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始在项目 {} 导入镜头帧范围", person_.person_.email_,
      person_.person_.get_full_name(), project_id_
  );
  auto l_project = l_sql.get_by_uuid<project>(project_id_);
  using namespace sqlite_orm;
  std::vector<std::string> l_shot_names;
  for (auto&& l_shot : l_args.shots_)
    if (!l_shot.name_.empty() && l_shot.name_.starts_with(l_project.code_))
      l_shot_names.emplace_back(
          l_shot.name_.substr(l_project.code_.size() + 1)
      );  // 镜头名称如果以项目 code 开头, 则去掉项目 code + "_" 前缀再匹配
    else
      throw_exception(
          http_request_error{boost::beast::http::status::bad_request, "镜头名称 " + l_shot.name_ + " 不合法"}
      );

  std::map<std::string, actions_projects_shots_import_frame_range_args::shot_frame_range_t*>
      l_shot_name_to_frame_range{};
  for (auto&& l_shot : l_args.shots_)
    l_shot_name_to_frame_range.emplace(l_shot.name_.substr(l_project.code_.size() + 1), &l_shot);

  auto sequence                            = "sequence"_alias.for_<entity>();
  constexpr orm_column_alias auto name_all = name_all_t{};
  auto l_list                              = l_sql.impl_->storage_any_.select(
      columns(conc(sequence->*&entity::name_, conc('_', &entity::name_)) >>= name_all, &entity::uuid_id_),
      from<entity>(), join<sequence>(on(c(&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
      where(c(&entity::project_id_) == project_id_ && in(sqlite_orm::get<name_all>(), l_shot_names))
  );
  auto l_shot_ext = std::make_shared<std::vector<entity_shot_extend>>();
  for (auto&& [l_shot_name, l_shot_id] : l_args.shots_) {
    if (l_shot_name_to_frame_range.contains(l_shot_name)) {
      auto&& l_frame_range = l_shot_name_to_frame_range[l_shot_name];
      auto l_shot_extend =
          l_sql.get_entity_shot_extend(l_shot_id).value_or(entity_shot_extend{.entity_id_ = l_shot_id});
      l_shot_extend.frame_in_  = l_frame_range->frame_in_;
      l_shot_extend.frame_out_ = l_frame_range->frame_out_;
      l_shot_ext->emplace_back(std::move(l_shot_extend));
    }
  }
  if (!l_shot_ext->empty()) co_await l_sql.update_range(l_shot_ext);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成在项目 {} 导入镜头帧范围", person_.person_.email_,
      person_.person_.get_full_name(), project_id_
  );

  co_return in_handle->make_msg_204();
}

}  // namespace doodle::http