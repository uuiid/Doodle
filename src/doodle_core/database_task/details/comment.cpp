#include "comment.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>

#include "metadata/comment.h"
#include "metadata/metadata.h"
#include <algorithm>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {
void sql_com<doodle::comment>::insert(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  auto& l_conn   = *in_ptr;
  auto l_handles = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  tables::comment l_table{};
  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.comment_string = sqlpp::parameter(l_table.comment_string),
      l_table.comment_time   = sqlpp::parameter(l_table.comment_time),
      l_table.entity_id      = sqlpp::parameter(l_table.entity_id)
  ));

  for (auto& l_h : l_handles) {
    auto& l_shot                = l_h.get<comment>();
    l_pre.params.comment_string = l_shot.p_comment;
    l_pre.params.comment_time   = l_shot.p_time_info;
    l_pre.params.entity_id      = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r                    = l_conn(l_pre);
    DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<comment>().get_name());
  }
}

void sql_com<doodle::comment>::update(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  auto& l_conn   = *in_ptr;
  auto l_handles = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  tables::comment l_table{};

  auto l_pre = l_conn.prepare(sqlpp::update(l_table)
                                  .set(
                                      l_table.comment_string = sqlpp::parameter(l_table.comment_string),
                                      l_table.comment_time   = sqlpp::parameter(l_table.comment_time)
                                  )
                                  .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id)));
  for (auto& l_h : l_handles) {
    auto& l_shot                = l_h.get<comment>();
    l_pre.params.comment_string = l_shot.p_comment;
    l_pre.params.comment_time   = l_shot.p_time_info;
    l_pre.params.entity_id      = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    auto l_r                    = l_conn(l_pre);
    DOODLE_LOG_INFO("更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<comment>().get_name());
  }
}
void sql_com<doodle::comment>::select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle) {
  auto& l_conn = *in_ptr;
  tables::comment l_table{};
  std::vector<comment> l_comment;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_comment.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row : l_conn(sqlpp::select(l_table.entity_id, l_table.comment_string, l_table.comment_time)
                              .from(l_table)
                              .where(l_table.entity_id.is_null()))) {
    comment l_s{};
    l_s.p_comment   = row.comment_string.value();
    l_s.p_time_info = row.comment_time.value();
    auto l_id       = row.entity_id.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_comment.emplace_back(std::move(l_s));
      l_entts.emplace_back(in_handle.at(l_id));
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  reg_->insert(l_entts.begin(), l_entts.end(), l_comment.begin());
}
void sql_com<doodle::comment>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::comment>(in_ptr, in_handle);
}

}  // namespace doodle::database_n