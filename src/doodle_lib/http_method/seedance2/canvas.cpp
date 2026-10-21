#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/seedance2/canvas_element.h>
#include <doodle_core/metadata/seedance2/canvas_link.h>
#include <doodle_core/metadata/seedance2/infinite_canvas.h>
#include <doodle_core/metadata/seedance2/subproject.h>

#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "core/global_function.h"
#include "reg.h"

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

/// 画布 ---------------------
// /api/seedance2/subproject/{subproject_id}/canvas
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql = get_sqlite_database();
  using namespace orm;
  auto l_result = select(l_sql)
                      .columns(object<sd2::infinite_canvas>())
                      .from<sd2::infinite_canvas>()
                      .where(c(&sd2::infinite_canvas::subproject_id_) == subproject_id_)
                      .order_by(&sd2::infinite_canvas::updated_at_, false)()
                      .to_vector();
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_, post) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_subproject          = l_sql.get_by_uuid<sd2::subproject>(subproject_id_);

  auto l_canvas              = std::make_shared<sd2::infinite_canvas>();
  l_canvas->uuid_id_         = uuid{};
  l_json.get_to(*l_canvas);
  l_canvas->subproject_id_   = subproject_id_;
  l_canvas->project_uuid_id_ = l_subproject.project_id_;
  l_canvas->user_id_         = person_.person_.uuid_id_;

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_sql).into<sd2::infinite_canvas>().values(*l_canvas));
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return in_handle->make_msg(nlohmann::json{} = *l_canvas);
}

// /api/seedance2/canvas/{canvas_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_instance, get) {
  auto l_sql    = get_sqlite_database();
  auto l_canvas = l_sql.get_by_uuid<sd2::infinite_canvas>(canvas_id_);
  person_.check_subproject_access(l_canvas.subproject_id_);

  using namespace orm;
  auto l_result = select(l_sql)
                      .columns(object<sd2::canvas_element>(), object<sd2::canvas_link>())
                      .from<sd2::canvas_element>()
                      .left_outer_join<sd2::canvas_link>(
                          &sd2::canvas_link::canvas_id_, &sd2::canvas_element::canvas_id_
                      )
                      .where(c(&sd2::canvas_element::canvas_id_) == canvas_id_)();

  std::map<uuid, sd2::canvas_element> l_nodes{};
  std::map<uuid, sd2::canvas_link> l_connections{};
  for (auto&& [l_element, l_link] : l_result) {
    l_nodes.try_emplace(l_element.uuid_id_, l_element);
    if (!l_link.uuid_id_.is_nil()) l_connections.try_emplace(l_link.uuid_id_, l_link);
  }

  auto l_node_values = l_nodes | std::views::values | std::ranges::to<std::vector>();
  auto l_link_values = l_connections | std::views::values | std::ranges::to<std::vector>();
  std::ranges::sort(l_node_values, [](const auto& l_lhs, const auto& l_rhs) {
    return l_lhs.created_at_.get_sys_time() < l_rhs.created_at_.get_sys_time();
  });
  std::ranges::sort(l_link_values, [](const auto& l_lhs, const auto& l_rhs) {
    return l_lhs.created_at_.get_sys_time() < l_rhs.created_at_.get_sys_time();
  });

  co_return in_handle->make_msg(
      nlohmann::json{{"canvas", l_canvas}, {"nodes", l_node_values}, {"connections", l_link_values}}
  );
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_instance, put) {
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_canvas = l_sql.get_by_uuid<sd2::infinite_canvas>(canvas_id_);
  person_.check_subproject_access(l_canvas.subproject_id_);
  person_.check_not_outsourcer();

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  auto l_update = update(l_sql).from<sd2::infinite_canvas>().set_from_ref<sd2::infinite_canvas>(l_json).where(
      c(&sd2::infinite_canvas::uuid_id_) == canvas_id_
  );
  l_sqls.emplace_back(std::move(l_update));
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_by_uuid<sd2::infinite_canvas>(canvas_id_));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_instance, delete_) {
  auto l_sql    = get_sqlite_database();
  auto l_canvas = l_sql.get_by_uuid<sd2::infinite_canvas>(canvas_id_);
  person_.check_subproject_access(l_canvas.subproject_id_);
  person_.check_not_outsourcer();

  if (!person_.is_producer() && l_canvas.user_id_ != person_.person_.uuid_id_)
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "只有本人、管理员或制片可以删除画布"});

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(
      delete_from(l_sql).from<sd2::infinite_canvas>().where(c(&sd2::infinite_canvas::uuid_id_) == canvas_id_)
  );
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return in_handle->make_msg_204();
}

}  // namespace doodle::http::seedance2
