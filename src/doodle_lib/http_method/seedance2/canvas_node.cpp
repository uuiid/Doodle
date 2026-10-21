#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/seedance2/canvas_element.h>
#include <doodle_core/metadata/seedance2/canvas_link.h>
#include <doodle_core/metadata/seedance2/infinite_canvas.h>

#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "core/global_function.h"
#include "reg.h"

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

/// 画布节点 ---------------------
// /api/seedance2/subproject/{subproject_id}/canvas/{canvas_id}/node
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_node, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql    = get_sqlite_database();
  auto l_canvas = l_sql.get_by_uuid<sd2::infinite_canvas>(canvas_id_);
  if (l_canvas.subproject_id_ != subproject_id_)
    throw_exception(http_request_error{boost::beast::http::status::not_found, "画布不存在"});

  using namespace orm;
  auto l_result = select(l_sql)
                      .columns(object<sd2::canvas_element>())
                      .from<sd2::canvas_element>()
                      .where(c(&sd2::canvas_element::canvas_id_) == canvas_id_)
                      .order_by(&sd2::canvas_element::created_at_, true)()
                      .to_vector();
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_node, post) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql    = get_sqlite_database();
  auto l_canvas = l_sql.get_by_uuid<sd2::infinite_canvas>(canvas_id_);
  if (l_canvas.subproject_id_ != subproject_id_)
    throw_exception(http_request_error{boost::beast::http::status::not_found, "画布不存在"});

  auto l_json            = in_handle->get_json();
  l_json["canvas_id"]    = canvas_id_;

  auto l_element      = std::make_shared<sd2::canvas_element>();
  l_json.get_to(*l_element);

  if (!l_element->parent_id_.is_nil()) {
    auto l_group = l_sql.get_by_uuid<sd2::canvas_element>(l_element->parent_id_);
    if (l_element->parent_id_ == l_element->uuid_id_)
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "group_id 不能指向自身"});
    if (l_group.canvas_id_ != canvas_id_ || l_group.element_type_ != sd2::canvas_element_type::group)
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "group_id 必须是同画布的 group 节点"});
  }

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_sql).into<sd2::canvas_element>().values(*l_element));
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return in_handle->make_msg(nlohmann::json{} = *l_element);
}

// /api/seedance2/subproject/{subproject_id}/canvas/{canvas_id}/node/{node_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_node_instance, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql     = get_sqlite_database();
  auto l_element = l_sql.get_by_uuid<sd2::canvas_element>(node_id_);
  if (l_element.canvas_id_ != canvas_id_)
    throw_exception(http_request_error{boost::beast::http::status::not_found, "节点不存在"});

  co_return in_handle->make_msg(nlohmann::json{} = l_element);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_node_instance, put) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_json    = in_handle->get_json();

  auto l_element = l_sql.get_by_uuid<sd2::canvas_element>(node_id_);
  if (l_element.canvas_id_ != canvas_id_)
    throw_exception(http_request_error{boost::beast::http::status::not_found, "节点不存在"});

  if (l_json.contains("group_id")) {
    auto l_group_id = l_json.at("group_id").get<uuid>();
    if (!l_group_id.is_nil()) {
      if (l_group_id == node_id_)
        throw_exception(http_request_error{boost::beast::http::status::bad_request, "group_id 不能指向自身"});
      auto l_group = l_sql.get_by_uuid<sd2::canvas_element>(l_group_id);
      if (l_group.canvas_id_ != canvas_id_ || l_group.element_type_ != sd2::canvas_element_type::group)
        throw_exception(
            http_request_error{boost::beast::http::status::bad_request, "group_id 必须是同画布的 group 节点"}
        );
    }
  }

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  auto l_update = update(l_sql).from<sd2::canvas_element>().set_from_ref<sd2::canvas_element>(l_json).where(
      c(&sd2::canvas_element::uuid_id_) == node_id_
  );
  l_sqls.emplace_back(std::move(l_update));
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_by_uuid<sd2::canvas_element>(node_id_));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_node_instance, delete_) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_element = l_sql.get_by_uuid<sd2::canvas_element>(node_id_);
  if (l_element.canvas_id_ != canvas_id_)
    throw_exception(http_request_error{boost::beast::http::status::not_found, "节点不存在"});

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(
      delete_from(l_sql).from<sd2::canvas_element>().where(c(&sd2::canvas_element::uuid_id_) == node_id_)
  );
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return in_handle->make_msg_204();
}

/// 画布连线 ---------------------
// /api/seedance2/subproject/{subproject_id}/canvas/{canvas_id}/connection
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_connection, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql    = get_sqlite_database();
  auto l_canvas = l_sql.get_by_uuid<sd2::infinite_canvas>(canvas_id_);
  if (l_canvas.subproject_id_ != subproject_id_)
    throw_exception(http_request_error{boost::beast::http::status::not_found, "画布不存在"});

  using namespace orm;
  auto l_result = select(l_sql)
                      .columns(object<sd2::canvas_link>())
                      .from<sd2::canvas_link>()
                      .where(c(&sd2::canvas_link::canvas_id_) == canvas_id_)
                      .order_by(&sd2::canvas_link::created_at_, true)()
                      .to_vector();
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_connection, post) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql    = get_sqlite_database();
  auto l_canvas = l_sql.get_by_uuid<sd2::infinite_canvas>(canvas_id_);
  if (l_canvas.subproject_id_ != subproject_id_)
    throw_exception(http_request_error{boost::beast::http::status::not_found, "画布不存在"});

  auto l_json         = in_handle->get_json();
  l_json["canvas_id"] = canvas_id_;

  auto l_link = std::make_shared<sd2::canvas_link>();
  l_json.get_to(*l_link);

  auto l_source = l_sql.get_by_uuid<sd2::canvas_element>(l_link->source_id_);
  if (l_source.canvas_id_ != canvas_id_)
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "from_node_id 必须是同画布的节点"});
  auto l_target = l_sql.get_by_uuid<sd2::canvas_element>(l_link->target_id_);
  if (l_target.canvas_id_ != canvas_id_)
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "to_node_id 必须是同画布的节点"});

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_sql).into<sd2::canvas_link>().values(*l_link));
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return in_handle->make_msg(nlohmann::json{} = *l_link);
}

// /api/seedance2/subproject/{subproject_id}/canvas/{canvas_id}/connection/{connection_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_connection_instance, delete_) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_link = l_sql.get_by_uuid<sd2::canvas_link>(connection_id_);
  if (l_link.canvas_id_ != canvas_id_)
    throw_exception(http_request_error{boost::beast::http::status::not_found, "连线不存在"});

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(
      delete_from(l_sql).from<sd2::canvas_link>().where(c(&sd2::canvas_link::uuid_id_) == connection_id_)
  );
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return in_handle->make_msg_204();
}

}  // namespace doodle::http::seedance2
