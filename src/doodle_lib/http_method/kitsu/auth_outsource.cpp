#include "doodle_core/core/core_set.h"
#include "doodle_core/metadata/computer.h"
#include "doodle_core/metadata/person.h"
#include "doodle_core/metadata/studio.h"
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/kitsu/kitsu_result.h>

#include "core/http/http_function.h"
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <vector>

namespace doodle::http {

namespace {
struct entity_outsource_studio_authorization : entity {
  std::vector<outsource_studio_authorization> authorizations_;

  std::optional<entity_asset_extend> entity_asset_extend_;
  std::optional<entity_shot_extend> entity_shot_extend_;
  std::string episode_name_;
  std::string sequence_name_;

  explicit entity_outsource_studio_authorization(const entity& in_entity) : entity(in_entity) {}

  // to json
  friend void to_json(nlohmann::json& j, const entity_outsource_studio_authorization& p) {
    to_json(j, static_cast<const entity&>(p));
    j["authorizations"] = p.authorizations_;
    if (p.entity_asset_extend_) to_json(j, *p.entity_asset_extend_);
    if (p.entity_shot_extend_) to_json(j, *p.entity_shot_extend_);
    j["episode_name"]  = p.episode_name_;
    j["sequence_name"] = p.sequence_name_;
  }

  static std::vector<entity_outsource_studio_authorization> get(const uuid& in_project_id) {
    auto l_sql = g_ctx().get<sqlite_database>();
    using namespace sqlite_orm;
    std::vector<entity_outsource_studio_authorization> l_ret{};
    l_ret.reserve(l_sql.get_project_entity_count(in_project_id));

    auto l_rows = l_sql.impl_->storage_any_.iterate(select(
        columns(
            object<entity>(true), object<outsource_studio_authorization>(true), object<entity_asset_extend>(true),
            object<entity_shot_extend>(true)
        ),
        from<entity>(),
        left_outer_join<outsource_studio_authorization>(
            on(c(&outsource_studio_authorization::entity_id_) == c(&entity::uuid_id_))
        ),
        left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
        left_outer_join<entity_shot_extend>(on(c(&entity_shot_extend::entity_id_) == c(&entity::uuid_id_))),
        where(c(&entity::project_id_) == in_project_id), order_by(&entity::name_)
    ));
    std::map<uuid, std::size_t> l_entity_map{};
    for (auto&& [l_entity, l_authorization, l_entity_asset_extend, l_entity_shot_extend] : l_rows) {
      if (!l_entity_map.contains(l_entity.uuid_id_)) {
        l_ret.emplace_back(entity_outsource_studio_authorization{l_entity});
        l_entity_map.emplace(l_entity.uuid_id_, l_ret.size() - 1);
      }
      if (l_authorization) l_ret[l_entity_map[l_entity.uuid_id_]].authorizations_.emplace_back(l_authorization);
      if (l_entity_asset_extend) l_ret[l_entity_map[l_entity.uuid_id_]].entity_asset_extend_ = l_entity_asset_extend;
      if (l_entity_shot_extend) l_ret[l_entity_map[l_entity.uuid_id_]].entity_shot_extend_ = l_entity_shot_extend;
    }
    auto l_sequence_type_id = l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_sequence}).uuid_id_;
    for (auto&& l_r : l_ret) {
      if (!l_r.parent_id_.is_nil()) {
        if (l_ret[l_entity_map[l_r.parent_id_]].entity_type_id_ == l_sequence_type_id) {
          l_ret[l_entity_map[l_r.uuid_id_]].episode_name_ = l_ret[l_entity_map[l_r.parent_id_]].name_;
        } else {
          l_ret[l_entity_map[l_r.uuid_id_]].sequence_name_ = l_ret[l_entity_map[l_r.parent_id_]].name_;
        }
      }
    }

    return l_ret;
  }
};

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_project_authorization, get) {
  person_.check_producer();
  auto l_sql       = g_ctx().get<sqlite_database>();
  auto l_auth_list = entity_outsource_studio_authorization::get(project_id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_auth_list);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_project_authorization, post) {
  person_.check_producer();

  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_auth = std::make_shared<outsource_studio_authorization>();
  in_handle->get_json().get_to(*l_auth);
  co_await l_sql.install(l_auth);
  co_return in_handle->make_msg(nlohmann::json{} = *l_auth);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_project_authorization_instance, get) {
  person_.check_producer();

  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_auth = l_sql.get_by_uuid<outsource_studio_authorization>(authorization_id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_auth);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_project_authorization_instance, delete_) {
  person_.check_producer();
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 实体外包授权 {}", person_.person_.email_,
      person_.person_.get_full_name(), authorization_id_
  );
  auto l_sql = g_ctx().get<sqlite_database>();
  co_await l_sql.remove<outsource_studio_authorization>(authorization_id_);
  co_return in_handle->make_msg_204();
}
}  // namespace doodle::http