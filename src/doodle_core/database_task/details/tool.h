#pragma once

#include "doodle_core/doodle_core_fwd.h"

#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
namespace doodle::database_n::detail {

template <typename T>
void sql_com_destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  auto& l_conn = *in_ptr;
  T l_tabl{};
  auto l_pre = l_conn.prepare(sqlpp::remove_from(l_tabl).where(l_tabl.entityId == sqlpp::parameter(l_tabl.entityId)));

  for (auto& l_id : in_handle) {
    l_pre.params.entityId = boost::numeric_cast<std::int64_t>(l_id);
    l_conn(l_pre);
  }
}

}  // namespace doodle::database_n::detail