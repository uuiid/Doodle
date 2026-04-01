//
// Created by TD on 24-12-30.
//

#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/working_file.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>

#include <boost/hana.hpp>
#include <boost/url/url.hpp>

#include <optional>
#include <spdlog/spdlog.h>
#include <sqlite_orm/sqlite_orm.h>


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
boost::asio::awaitable<boost::beast::http::message_generator> projects_assets_new::post(session_data_ptr in_handle) {
  projects_assets_new_post_data l_data{};
  l_data.project_id = project_id_;
  person_.check_project_manager(l_data.project_id);
  l_data.asset_type_id = asset_type_id_;
  auto l_json          = in_handle->get_json();
  l_json.get_to(l_data);
  default_logger_raw()->info("由 {} , {} 项目创建新实体 {}", person_.person_.email_, l_data.project_id, l_data.name);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始在项目 {} 创建资产 name {} asset_type_id {}", person_.person_.email_,
      person_.person_.get_full_name(), l_data.project_id, l_data.name, l_data.asset_type_id
  );

  auto l_entity = std::make_shared<entity>(entity{
      .name_           = l_data.name,
      .description_    = l_data.description,
      .is_shared_      = l_data.is_shared,
      .status_         = entity_status::running,
      .project_id_     = l_data.project_id,
      .entity_type_id_ = l_data.asset_type_id,
      .source_id_      = l_data.source_id,
      .created_by_     = person_.person_.uuid_id_,
  });
  auto l_sql    = get_sqlite_database();
  co_await l_sql.install(l_entity);
  nlohmann::json l_json_ret{};
  l_json_ret = *l_entity;
  if (entity_asset_extend::has_extend_data(l_json)) {
    auto l_entity_extend = std::make_shared<entity_asset_extend>(entity_asset_extend{
        .entity_id_ = l_entity->uuid_id_,
    });
    l_json.get_to(*l_entity_extend);
    co_await l_sql.install(l_entity_extend);
    // l_json_ret = *l_entity_extend;
    l_json_ret.update(*l_entity_extend);
  }
  socket_io::broadcast(
      socket_io::asset_new_broadcast_t{
          .asset_id_   = l_entity->uuid_id_,
          .asset_type_ = l_entity->entity_type_id_,
          .project_id_ = l_entity->project_id_
      }
  );

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成在项目 {} 创建资产 asset_id {} asset_type_id {}",
      person_.person_.email_, person_.person_.get_full_name(), l_entity->project_id_, l_entity->uuid_id_,
      l_entity->entity_type_id_
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
        kai_shi_ji_shu_(in_asset_extend.kai_shi_ji_shu_),
        chang_ci_(in_asset_extend.chang_ci_) {}

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
  decltype(entity_asset_extend::chang_ci_) chang_ci_;

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
    std::vector<working_file> working_files_;
    bool is_subscribed_;
    friend void to_json(nlohmann::json& j, const task_t& p) {
      j["id"]                   = p.uuid_id_;
      j["due_date"]             = p.due_date_;
      j["done_date"]            = p.done_date_;
      j["duration"]             = p.duration_;
      j["entity_id"]            = p.entity_id_;
      j["estimation"]           = p.estimation_;
      j["end_date"]             = p.end_date_;
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
      j["is_subscribed"]        = p.is_subscribed_;
      j["working_files"]        = p.working_files_;
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
    j["chang_ci"]            = p.chang_ci_;
  }
};

struct make_with_tasks_sql_result_t {
  person& person_;
  uuid project_id_;
  uuid id_;
  std::int32_t offset_{0};
  std::int32_t limit_{300};
  std::vector<uuid> entity_type_id_;
  std::vector<std::int32_t> ji_du_filter_;
  std::vector<std::int32_t> ji_shu_lie_filter_;
  std::vector<uuid> task_status_id_filter_;
  std::vector<uuid> person_id_filter_;

 private:
  auto project_id_where() {
    using namespace sqlite_orm;
    return c(&entity::project_id_) == project_id_;
  }
  auto outsource_where() {
    using namespace sqlite_orm;

    return in(
        &entity::uuid_id_, select(
                               &outsource_studio_authorization::entity_id_,
                               where(c(&outsource_studio_authorization::studio_id_) == person_.studio_id_)
                           )
    );
  }

  auto entity_type_id_where() {
    using namespace sqlite_orm;
    return in(&entity::entity_type_id_, entity_type_id_);
  }
  auto ji_du_where() {
    using namespace sqlite_orm;
    return in(&entity_asset_extend::ji_du_, ji_du_filter_);
  }
  auto ji_shu_lie_where() {
    using namespace sqlite_orm;
    return in(&entity_asset_extend::ji_shu_lie_, ji_shu_lie_filter_);
  }
  auto task_status_id_where() {
    using namespace sqlite_orm;
    return in(&task::task_status_id_, task_status_id_filter_);
  }
  auto person_id_where() {
    using namespace sqlite_orm;
    return in(&assignees_table::person_id_, person_id_filter_);
  }

  template <typename T>
  auto with_tasks_sql_query(T&& in_where) {
    auto l_sql = get_sqlite_database();
    std::vector<with_tasks_get_result_t> l_ret{};

    using namespace sqlite_orm;

    l_ret.reserve(l_sql.get_project_entity_count(project_id_));

    auto l_subscriptions_for_user = l_sql.get_person_subscriptions(person_, project_id_, {});

    auto l_outsource_select       = select(
        &outsource_studio_authorization::entity_id_,
        where(c(&outsource_studio_authorization::studio_id_) == person_.studio_id_)
    );

    auto l_rows = l_sql.impl_->storage_any_.iterate(select(
        columns(
            object<entity>(true), object<task>(true), object<entity_asset_extend>(true), object<asset_type>(true),
            &assignees_table::person_id_
        ),
        from<entity>(), join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
        left_outer_join<task>(on(c(&entity::uuid_id_) == c(&task::entity_id_))),
        left_outer_join<assignees_table>(on(c(&assignees_table::task_id_) == c(&task::uuid_id_))),
        left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
        where(std::forward<T>(in_where)), multi_order_by(order_by(&asset_type::name_), order_by(&entity::name_))
    ));
    std::map<uuid, std::size_t> l_entities_and_tasks_map{};
    std::map<uuid, std::size_t> l_task_id_set{};
    for (auto&& [l_entity, l_task, l_entity_asset_extend, l_asset_type, l_person_id] : l_rows) {
      if (!l_entities_and_tasks_map.contains(l_entity.uuid_id_)) {
        l_ret.emplace_back(with_tasks_get_result_t{l_entity, l_entity_asset_extend, l_asset_type});
        l_entities_and_tasks_map.emplace(l_entity.uuid_id_, l_ret.size() - 1);
      }
      if (!l_task.uuid_id_.is_nil()) {
        if (!l_task_id_set.contains(l_task.uuid_id_)) {
          l_ret[l_entities_and_tasks_map[l_entity.uuid_id_]].tasks_.emplace_back(
              with_tasks_get_result_t::task_t{
                  l_entity,
                  l_task,
                  l_subscriptions_for_user.contains(l_task.uuid_id_),
              }
          );
          l_task_id_set.emplace(l_task.uuid_id_, l_ret[l_entities_and_tasks_map[l_entity.uuid_id_]].tasks_.size() - 1);
        }
        if (!l_person_id.is_nil())
          l_ret[l_entities_and_tasks_map[l_entity.uuid_id_]]
              .tasks_[l_task_id_set.at(l_task.uuid_id_)]
              .assigners_.emplace_back(l_person_id);
      }
    }
    return l_ret;
  }

 public:
  explicit make_with_tasks_sql_result_t(person& in_person, const boost::urls::url& in_url, const uuid& in_id)
      : person_(in_person), id_(in_id) {
    for (auto&& l_i : in_url.params()) {
      if (l_i.key == "entity_type_id") entity_type_id_.emplace_back(from_uuid_str(l_i.value));
      if (l_i.key == "ji_du") ji_du_filter_.emplace_back(std::stoi(l_i.value));
      if (l_i.key == "ji_shu_lie") ji_shu_lie_filter_.emplace_back(std::stoi(l_i.value));
      if (l_i.key == "task_status_id") task_status_id_filter_.emplace_back(from_uuid_str(l_i.value));
      if (l_i.key == "person_id") person_id_filter_.emplace_back(from_uuid_str(l_i.value));
      if (l_i.key == "offset") offset_ = std::stoi(l_i.value);
      if (l_i.key == "limit") limit_ = std::stoi(l_i.value);
      if (l_i.key == "project_id") project_id_ = from_uuid_str(l_i.value);
    }
  }

  auto operator()() {
    using namespace sqlite_orm;

    auto l_sql               = get_sqlite_database();
    auto l_temporal_type_ids = l_sql.get_temporal_type_ids();

    auto l_dynamic_where     = dynamic_where(l_sql.impl_->storage_any_);
    l_dynamic_where.push_back(not_in(&entity::entity_type_id_, l_temporal_type_ids));
    if (person_.role_ == person_role_type::outsource) l_dynamic_where.push_back(outsource_where());
    if (!id_.is_nil()) {
      l_dynamic_where.push_back(c(&entity::uuid_id_) == id_);
      return with_tasks_sql_query(l_dynamic_where);
    }
    // 0 位 project id
    // 1 位 outsource
    // 2 位 entity type id
    // 3 位 ji du
    // 4 位 ji shu lie
    // 5 位 task status id
    // 6 位 person id
    if (!project_id_.is_nil()) l_dynamic_where.push_back(project_id_where());
    if (!entity_type_id_.empty()) l_dynamic_where.push_back(entity_type_id_where());
    if (!ji_du_filter_.empty()) l_dynamic_where.push_back(ji_du_where());
    if (!ji_shu_lie_filter_.empty()) l_dynamic_where.push_back(ji_shu_lie_where());
    if (!task_status_id_filter_.empty()) l_dynamic_where.push_back(task_status_id_where());
    if (!person_id_filter_.empty()) l_dynamic_where.push_back(person_id_where());

    return with_tasks_sql_query(l_dynamic_where);
  }
};

}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> data_assets_with_tasks::get(session_data_ptr in_handle) {
  make_with_tasks_sql_result_t l_make_result(person_.person_, in_handle->url_, {});

  co_return in_handle->make_msg(nlohmann::json{} = l_make_result());
}
boost::asio::awaitable<boost::beast::http::message_generator> asset_details::get(session_data_ptr in_handle) {
  auto&& l_sql = get_sqlite_database();
  make_with_tasks_sql_result_t l_make_result(person_.person_, in_handle->url_, id_);
  auto l_t = l_make_result();
  if (l_t.empty())
    throw_exception(http_request_error{boost::beast::http::status::not_found, fmt::format("未找到资源 {}", id_)});
  auto l_ass                = l_sql.get_by_uuid<entity>(id_);
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
boost::asio::awaitable<boost::beast::http::message_generator> asset_details::delete_(session_data_ptr in_handle) {
  auto l_sql = get_sqlite_database();
  auto l_ass = std::make_shared<entity>(l_sql.get_by_uuid<entity>(id_));
  person_.check_delete_access(l_ass->project_id_);
  bool l_force{};
  for (auto&& l_i : in_handle->url_.params()) {
    if (l_i.key == "force") l_force = true;
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除实体 {}", person_.person_.email_, person_.person_.get_full_name(),
      l_ass->uuid_id_
  );
  if (!l_force) {
    l_ass->canceled_ = true;
    co_await l_sql.update(l_ass);
    co_return in_handle->make_msg(nlohmann::json{} = *l_ass);
  }
  auto l_task     = l_sql.get_tasks_for_entity(l_ass->uuid_id_);
  auto l_task_ids = l_task | ranges::views::transform([](const task& in) { return in.uuid_id_; }) | ranges::to_vector;
  co_await l_sql.remove<task>(l_task_ids);
  co_await l_sql.remove<entity>(l_ass->uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_ass);
}
boost::asio::awaitable<boost::beast::http::message_generator> shared_used::get(session_data_ptr in_handle) {
  co_return in_handle->make_msg(nlohmann::json::array());
}

boost::asio::awaitable<boost::beast::http::message_generator> data_assets_cast_in::get(session_data_ptr in_handle) {
  co_return in_handle->make_msg(nlohmann::json::array());
}
boost::asio::awaitable<boost::beast::http::message_generator> data_assets::get(session_data_ptr in_handle) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_temporal_type_ids = l_sql.get_temporal_type_ids();
  bool l_is_shared{};
  for (auto&& [key, value, has] : in_handle->url_.params())
    if (key == "is_shared") l_is_shared = true;
  auto l_project_link_ids = l_sql.impl_->storage_any_.select(
      &project_person_link::project_id_, where(c(&project_person_link::person_id_) == person_.person_.uuid_id_)
  );
  auto l_entt = l_sql.impl_->storage_any_.get_all<entity>(where(
      not_in(&entity::entity_type_id_, l_temporal_type_ids) &&
      (person_.is_admin() || in(&entity::project_id_, l_project_link_ids)) && c(&entity::is_shared_) == l_is_shared
  ));
  co_return in_handle->make_msg(nlohmann::json{} = l_entt);
}

}  // namespace doodle::http
