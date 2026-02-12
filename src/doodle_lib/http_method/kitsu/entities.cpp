//
// Created by TD on 25-6-16.
//
#include "doodle_core/metadata/entity.h"
#include "doodle_core/sqlite_orm/detail/sqlite_database_impl.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"

#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
namespace {}
boost::asio::awaitable<boost::beast::http::message_generator> data_entities::put(session_data_ptr in_handle) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_entt = std::make_shared<entity>(l_sql.get_by_uuid<entity>(id_));
  auto l_json = in_handle->get_json();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始更新实体 entity_id {} project_id {}", person_.person_.email_,
      person_.person_.get_full_name(), l_entt->uuid_id_, l_entt->project_id_
  );
  l_json.get_to(*l_entt);
  nlohmann::json l_res{};
  const bool l_can_update = person_.is_project_supervisor(l_entt->project_id_);
  if (l_can_update) co_await l_sql.update(l_entt);
  l_res = *l_entt;
  if (entity_asset_extend::has_extend_data(l_json)) {
    using namespace sqlite_orm;
    auto l_ext_ptr = std::make_shared<entity_asset_extend>();
    if (auto l_list_ext = l_sql.impl_->storage_any_.get_all<entity_asset_extend>(
            where(c(&entity_asset_extend::entity_id_) == l_entt->uuid_id_)
        );
        !l_list_ext.empty()) {
      *l_ext_ptr = l_list_ext.front();
      l_json.get_to(*l_ext_ptr);
      co_await l_sql.update(l_ext_ptr);
    } else {
      l_json.get_to(*l_ext_ptr);
      l_ext_ptr->entity_id_ = l_entt->uuid_id_;
      co_await l_sql.install(l_ext_ptr);
    }
    l_res.update(*l_ext_ptr);
  }
  if (entity_shot_extend::has_extend_data(l_json)) {
    using namespace sqlite_orm;
    auto l_ext_ptr = std::make_shared<entity_shot_extend>();
    if (auto l_list_ext = l_sql.impl_->storage_any_.get_all<entity_shot_extend>(
            where(c(&entity_shot_extend::entity_id_) == l_entt->uuid_id_)
        );
        !l_list_ext.empty()) {
      *l_ext_ptr = l_list_ext.front();
      l_json.get_to(*l_ext_ptr);
      co_await l_sql.update(l_ext_ptr);
    } else {
      l_json.get_to(*l_ext_ptr);
      l_ext_ptr->entity_id_ = l_entt->uuid_id_;
      co_await l_sql.install(l_ext_ptr);
    }
    l_res.update(*l_ext_ptr);
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成更新实体 entity_id {} project_id {} updated {}", person_.person_.email_,
      person_.person_.get_full_name(), l_entt->uuid_id_, l_entt->project_id_, l_can_update
  );

  co_return in_handle->make_msg(l_res);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_entities_news::get(session_data_ptr in_handle) {
  co_return in_handle->make_msg(
      nlohmann::json{
          {"data", nlohmann::json::array()}, {"limit", 2000}, {"nb_pages", 0}, {"offset", 0}, {"page", 1}, {"total", 0}
      }
  );
}

}  // namespace doodle::http