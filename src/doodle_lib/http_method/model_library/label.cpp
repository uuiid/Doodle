//
// Created by TD on 25-5-8.
//
#include <doodle_core/metadata/label.h>

#include <doodle_core/metadata/assets_file.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "model_library.h"

namespace doodle::http::model_library {

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(assets_tree_link, post) {
  auto l_sql = get_sqlite_database();
  if (l_sql.has_assets_tree_assets_link(id_, assets_id_)) co_return in_handle->make_msg(nlohmann::json{});
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 在 资产库节点 {} 中添加 资产库文件 {} ", person_.person_.email_,
      person_.person_.get_full_name(), id_, assets_id_
  );
  auto l_link               = std::make_shared<assets_file_helper::link_parent_t>();
  l_link->assets_type_uuid_ = id_;
  l_link->assets_uuid_      = assets_id_;
  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_sql).into<assets_file_helper::link_parent_t>().values(*l_link));
  co_await l_sql.run_sql(std::move(l_sqls));
  co_return in_handle->make_msg(nlohmann::json{});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(assets_tree_link, delete_) {
  auto l_sql = get_sqlite_database();
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 从 资产库节点 {} 移除 资产库文件 {} ", person_.person_.email_,
      person_.person_.get_full_name(), id_, assets_id_
  );
  if (!l_sql.has_assets_tree_assets_link(id_, assets_id_)) co_return in_handle->make_msg(nlohmann::json{});
  auto l_link = l_sql.get_assets_tree_assets_link(id_, assets_id_);
  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(
      delete_from(l_sql).from<assets_file_helper::link_parent_t>().where(
          c(&assets_file_helper::link_parent_t::id_) == l_link.id_
      )
  );
  co_await l_sql.run_sql(std::move(l_sqls));
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http::model_library