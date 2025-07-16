//
// Created by TD on 25-6-16.
//
#include "doodle_core/metadata/entity.h"
#include "doodle_core/sqlite_orm/detail/sqlite_database_impl.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"

#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
namespace {}
boost::asio::awaitable<boost::beast::http::message_generator> data_entities_put::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_p    = get_person(in_handle);
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_entt = std::make_shared<entity>(l_sql.get_by_uuid<entity>(in_arg->id_));
  auto l_json = in_handle->get_json();

  l_json.get_to(*l_entt);
  nlohmann::json l_res{};
  co_await l_sql.install(l_entt);
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
    } else {
      l_json.get_to(*l_ext_ptr);
      l_ext_ptr->entity_id_ = l_entt->uuid_id_;
      l_ext_ptr->uuid_id_   = core_set::get_set().get_uuid();
    }
    co_await l_sql.impl_->install(l_ext_ptr);
    l_res.update(*l_ext_ptr);
  }

  co_return in_handle->make_msg(l_res);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_entities_news_get::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  co_return in_handle->make_msg(
      nlohmann::json{
          {"data", nlohmann::json::array()}, {"limit", 2000}, {"nb_pages", 0}, {"offset", 0}, {"page", 1}, {"total", 0}
      }
  );
}

}  // namespace doodle::http