//
// Created by TD on 24-12-30.
//

#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "kitsu.h"

namespace doodle::http {
namespace {
struct projects_assets_new_post_data {
  nlohmann::json data;
  std::string description;
  bool is_shared;
  std::string name;
  uuid source_id;

  uuid project_id;
  uuid asset_type_id;

  // form json
  friend void from_json(const nlohmann::json& j, projects_assets_new_post_data& p) {
    j.at("description").get_to(p.description);
    j.at("is_shared").get_to(p.is_shared);
    j.at("name").get_to(p.name);
  }
};
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> projects_assets_new_post::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<projects_assets_new_arg> in_arg
) {
  auto l_ptr = get_person(in_handle);
  projects_assets_new_post_data l_data{};
  l_data.project_id = in_arg->project_id;
  l_ptr->is_project_manager(l_data.project_id);
  l_data.asset_type_id = in_arg->asset_type_id;
  auto l_json          = in_handle->get_json();
  l_json.get_to(l_data);

  auto l_entity = std::make_shared<entity>(entity{
      .uuid_id_        = core_set::get_set().get_uuid(),
      .name_           = l_data.name,
      .description_    = l_data.description,
      .is_shared_      = l_data.is_shared,
      .status_         = entity_status::running,
      .project_id_     = l_data.project_id,
      .entity_type_id_ = l_data.asset_type_id,
      .source_id_      = l_data.source_id,
      .created_by_     = l_ptr->person_.uuid_id_,
  });
  auto l_sql    = g_ctx().get<sqlite_database>();
  co_await l_sql.install(l_entity);
  nlohmann::json l_json_ret{};
  l_json_ret = *l_entity;
  if (entity_asset_extend::has_extend_data(l_json)) {
    auto l_entity_extend = std::make_shared<entity_asset_extend>(entity_asset_extend{
        .uuid_id_   = core_set::get_set().get_uuid(),
        .entity_id_ = l_entity->uuid_id_,
    });
    l_json.get_to(*l_entity_extend);
    co_await l_sql.install(l_entity_extend);
    // l_json_ret = *l_entity_extend;
    l_json_ret.update(*l_entity_extend);
  }

  socket_io::broadcast(
      "asset:new",
      nlohmann::json{
          {"asset_id", l_entity->uuid_id_},
          {"asset_type", l_entity->entity_type_id_},
          {"project_id", l_entity->project_id_}
      },
      "/events"
  );
  co_return in_handle->make_msg(l_json_ret);
}

namespace {

struct with_tasks_get_result_t {
  with_tasks_get_result_t() = default;
  explicit with_tasks_get_result_t(
      const entity& in_entity, const entity_asset_extend& in_asset_extend, const asset_type& in_asset_type
  )
      : uuid_id_(in_entity.uuid_id_),
        name_(in_entity.name_),
        preview_file_id_(in_entity.preview_file_id_),
        description_(in_entity.description_),
        asset_type_name_(in_asset_type.name_),
        asset_type_id_(in_asset_type.uuid_id_),
        canceled_(in_entity.canceled_),
        ready_for_(in_entity.ready_for_),
        source_id_(in_entity.source_id_),
        is_casting_standby_(in_entity.is_casting_standby_),
        is_shared_(in_entity.is_shared_),

        ji_shu_lie_(in_asset_extend.ji_shu_lie_),
        deng_ji_(in_asset_extend.deng_ji_),
        gui_dang_(in_asset_extend.gui_dang_),
        bian_hao_(in_asset_extend.bian_hao_),
        pin_yin_ming_cheng_(in_asset_extend.pin_yin_ming_cheng_),
        ban_ben_(in_asset_extend.ban_ben_),
        ji_du_(in_asset_extend.ji_du_),
        kai_shi_ji_shu_(in_asset_extend.kai_shi_ji_shu_) {}

  decltype(entity::uuid_id_) uuid_id_;
  decltype(entity::name_) name_;
  decltype(entity::preview_file_id_) preview_file_id_;
  decltype(entity::description_) description_;
  decltype(asset_type::name_) asset_type_name_;
  decltype(asset_type::uuid_id_) asset_type_id_;
  decltype(entity::canceled_) canceled_;
  decltype(entity::ready_for_) ready_for_;
  decltype(entity::source_id_) source_id_;
  decltype(entity::is_casting_standby_) is_casting_standby_;
  decltype(entity::is_shared_) is_shared_;
  std::vector<uuid> casting_episode_ids_;

  // 额外的资产数据
  decltype(entity_asset_extend::ji_shu_lie_) ji_shu_lie_;
  decltype(entity_asset_extend::deng_ji_) deng_ji_;
  decltype(entity_asset_extend::gui_dang_) gui_dang_;
  decltype(entity_asset_extend::bian_hao_) bian_hao_;
  decltype(entity_asset_extend::pin_yin_ming_cheng_) pin_yin_ming_cheng_;
  decltype(entity_asset_extend::ban_ben_) ban_ben_;
  decltype(entity_asset_extend::ji_du_) ji_du_;
  decltype(entity_asset_extend::kai_shi_ji_shu_) kai_shi_ji_shu_;

  struct task_t {
    task_t() = default;
    explicit task_t(const entity& in_entity, const task& in_task, bool in_is_subscribed)
        : uuid_id_(in_task.uuid_id_),
          estimation_(in_task.estimation_),
          entity_id_(in_entity.uuid_id_),
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
          assigners_(),
          is_subscribed_(in_is_subscribed)

    {}

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
      j["id"]                = p.uuid_id_;
      j["due_date"]          = p.due_date_;
      j["done_date"]         = p.done_date_;
      j["duration"]          = p.duration_;
      j["entity_id"]         = p.entity_id_;
      j["estimation"]        = p.estimation_;
      j["end_date"]          = p.end_date_;
      j["last_comment_date"] = p.last_comment_date_;
      j["last_preview_file"] = p.last_preview_file_id_;
      j["priority"]          = p.priority_;
      j["real_start_date"]   = p.real_start_date_;
      j["retake_count"]      = p.retake_count_;
      j["start_date"]        = p.start_date_;
      j["difficulty"]        = p.difficulty_;
      j["task_type_id"]      = p.task_type_id_;
      j["task_status_id"]    = p.task_status_id_;
      j["assignees"]         = p.assigners_;
      j["is_subscribed"]     = p.is_subscribed_;
      j["path"]              = "";
    }
  };
  std::vector<task_t> tasks_;
  // to json
  friend void to_json(nlohmann::json& j, const with_tasks_get_result_t& p) {
    j["id"]                  = p.uuid_id_;
    j["name"]                = p.name_;
    j["preview_file_id"]     = p.preview_file_id_;
    j["description"]         = p.description_;
    j["asset_type_name"]     = p.asset_type_name_;
    j["asset_type_id"]       = p.asset_type_id_;
    j["canceled"]            = p.canceled_;
    j["ready_for"]           = p.ready_for_;
    j["source_id"]           = p.source_id_;
    j["is_casting_standby"]  = p.is_casting_standby_;
    j["is_shared"]           = p.is_shared_;
    j["casting_episode_ids"] = p.casting_episode_ids_;
    j["tasks"]               = p.tasks_;

    j["ji_shu_lie"]          = p.ji_shu_lie_;
    j["deng_ji"]             = p.deng_ji_;
    j["gui_dang"]            = p.gui_dang_;
    j["bian_hao"]            = p.bian_hao_;
    j["pin_yin_ming_cheng"]  = p.pin_yin_ming_cheng_;
    j["ban_ben"]             = p.ban_ben_;
    j["ji_du"]               = p.ji_du_;
    j["kai_shi_ji_shu"]      = p.kai_shi_ji_shu_;
  }
};

auto with_tasks_sql_query(const person& in_person, const uuid& in_project_id, const uuid& in_id) {
  auto l_sql = g_ctx().get<sqlite_database>();
  std::vector<with_tasks_get_result_t> l_ret{};
  using namespace sqlite_orm;
  auto l_subscriptions_for_user = l_sql.get_person_subscriptions(in_person, in_project_id, {});
  auto l_rows                   = l_sql.impl_->storage_any_.select(
      columns(
          object<entity>(true), object<task>(true), object<entity_asset_extend>(true), object<asset_type>(true),
          &assignees_table::person_id_
      ),
      join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
      left_outer_join<task>(on(c(&entity::uuid_id_) == c(&task::entity_id_))),
      left_outer_join<assignees_table>(on(c(&assignees_table::task_id_) == c(&task::uuid_id_))),
      left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
      where(
          ((!in_id.is_nil() && c(&entity::uuid_id_) == in_id) ||
           (!in_project_id.is_nil() && c(&entity::project_id_) == in_project_id)) &&
          not_in(&entity::entity_type_id_, l_sql.get_temporal_type_ids())
      ),
      multi_order_by(order_by(&asset_type::name_), order_by(&entity::name_))
  );
  std::map<uuid, with_tasks_get_result_t> l_entities_and_tasks_map{};
  std::map<uuid, std::size_t> l_task_id_set{};
  for (auto&& [l_entity, l_task, l_entity_asset_extend, l_asset_type, l_person_id] : l_rows) {
    if (!l_entities_and_tasks_map.contains(l_entity.uuid_id_)) {
      l_entities_and_tasks_map.emplace(
          l_entity.uuid_id_, with_tasks_get_result_t{l_entity, l_entity_asset_extend, l_asset_type}
      );
    }
    if (!l_task.uuid_id_.is_nil()) {
      if (!l_task_id_set.contains(l_task.uuid_id_)) {
        l_entities_and_tasks_map[l_entity.uuid_id_].tasks_.emplace_back(
            with_tasks_get_result_t::task_t{
                l_entity,
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

boost::asio::awaitable<boost::beast::http::message_generator> with_tasks_get::callback_arg(session_data_ptr in_handle) {
  auto l_ptr = get_person(in_handle);
  uuid l_prj_id{};
  for (auto&& l_i : in_handle->url_.params())
    if (l_i.key == "project_id") l_prj_id = from_uuid_str(l_i.value);
  co_return in_handle->make_msg((nlohmann::json{} = with_tasks_sql_query(l_ptr->person_, l_prj_id, {})).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> asset_details_get::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_ptr   = get_person(in_handle);
  auto&& l_sql = g_ctx().get<sqlite_database>();
  auto l_t     = with_tasks_sql_query(l_ptr->person_, {}, in_arg->id_);
  if (l_t.empty())
    throw_exception(
        http_request_error{boost::beast::http::status::not_found, fmt::format("未找到资源 {}", in_arg->id_)}
    );
  auto l_ass                = l_sql.get_by_uuid<entity>(in_arg->id_);
  auto l_project            = l_sql.get_by_uuid<project>(l_ass.project_id_);
  auto l_ass_type           = l_sql.get_by_uuid<asset_type>(l_ass.entity_type_id_);

  auto l_json               = nlohmann::json{};
  l_json                    = l_ass;
  l_json["project_name"]    = l_project.name_;
  l_json["asset_type_id"]   = l_ass_type.uuid_id_;
  l_json["asset_type_name"] = l_ass_type.name_;
  l_json.update(l_t[0]);
  co_return in_handle->make_msg(l_json);
}

boost::asio::awaitable<boost::beast::http::message_generator> shared_used_get::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  get_person(in_handle);
  co_return in_handle->make_msg(nlohmann::json::array());
}

boost::asio::awaitable<boost::beast::http::message_generator> data_asset_delete_::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_ptr = get_person(in_handle);
  auto l_sql = g_ctx().get<sqlite_database>();
  auto l_ass = std::make_shared<entity>(l_sql.get_by_uuid<entity>(in_arg->id_));
  l_ptr->check_delete_access(l_ass->project_id_);
  bool l_force{};
  for (auto&& l_i : in_handle->url_.params()) {
    if (l_i.key == "force") l_force = true;
  }
  if (!l_force) {
    l_ass->canceled_ = true;
    co_await l_sql.install(l_ass);
    co_return in_handle->make_msg(nlohmann::json{} = *l_ass);
  }
  auto l_task     = l_sql.get_tasks_for_entity(l_ass->uuid_id_);
  auto l_task_ids = l_task | ranges::views::transform([](const task& in) { return in.uuid_id_; }) | ranges::to_vector;
  co_await l_sql.remove<task>(l_task_ids);
  co_await l_sql.remove<entity>(l_ass->uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_ass);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_assets_cast_in_get::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  co_return in_handle->make_msg(nlohmann::json::array());
}

}  // namespace doodle::http
