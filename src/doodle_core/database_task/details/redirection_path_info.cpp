#include "redirection_path_info.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>

#include <boost/filesystem/path.hpp>
#include <boost/none.hpp>

#include "core/core_help_impl.h"
#include "metadata/metadata.h"
#include "metadata/redirection_path_info.h"
#include <algorithm>
#include <cstdint>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <map>
#include <memory>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/insert.h>
#include <sqlpp11/parameter.h>
#include <sqlpp11/remove.h>
#include <sqlpp11/select.h>
#include <sqlpp11/single_table.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/update.h>
#include <string>
#include <utility>
#include <vector>

namespace doodle::database_n {
void sql_com<doodle::redirection_path_info>::insert(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  auto& l_conn   = *in_ptr;
  auto l_handles = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  tables::redirection_path_info l_table{};
  tables::rpi_search_path l_path_table{};
  std::map<entt::handle, std::int64_t> path_map{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.redirection_file_name = sqlpp::parameter(l_table.redirection_file_name),
      l_table.entity_id             = sqlpp::parameter(l_table.entity_id)
  ));
  for (auto& l_h : l_handles) {
    auto& l_r_p_i                      = l_h.get<doodle::redirection_path_info>();
    l_pre.params.redirection_file_name = l_r_p_i.file_name_.generic_string();
    l_pre.params.entity_id             = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    auto l_r                           = l_conn(l_pre);
    path_map.emplace(l_h, l_r);
    DOODLE_LOG_INFO(
        "插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<redirection_path_info>().get_name()
    );
  }

  auto l_path_pre =
      l_conn.prepare(sqlpp::insert_into(l_path_table)
                         .set(
                             l_path_table.parent_id        = sqlpp::parameter(l_path_table.parent_id),
                             l_path_table.redirection_path = sqlpp::parameter(l_path_table.redirection_path)
                         ));
  for (auto& l_h : l_handles) {
    auto& l_r_p_i = l_h.get<doodle::redirection_path_info>();
    for (auto& l_p : l_r_p_i.search_path_) {
      l_path_pre.params.parent_id        = path_map[l_h];
      l_path_pre.params.redirection_path = l_p.generic_string();
    }
    auto l_r_p = l_conn(l_path_pre);
    DOODLE_LOG_INFO(
        "插入数据库id {} -> 实体 {} 组件 {} ", l_r_p, l_h.entity(),
        rttr::type::get<tables::rpi_search_path>().get_name()
    );
  }
}

void sql_com<doodle::redirection_path_info>::update(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  auto& l_conn   = *in_ptr;
  auto l_handles = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  tables::redirection_path_info l_table{};
  tables::rpi_search_path l_path_table{};
  std::map<entt::handle, std::int64_t> path_map{};
  std::vector<std::int64_t> path_handle{};
  auto l_pre = l_conn.prepare(sqlpp::update(l_table)
                                  .set(l_table.redirection_file_name = sqlpp::parameter(l_table.redirection_file_name))
                                  .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id)));
  for (auto& l_h : l_handles) {
    auto& l_r_p_i                      = l_h.get<doodle::redirection_path_info>();
    l_pre.params.redirection_file_name = l_r_p_i.file_name_.generic_string();
    l_pre.params.entity_id             = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r                           = l_conn(l_pre);
    DOODLE_LOG_INFO(
        "插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<redirection_path_info>().get_name()
    );
  }

  auto l_pre_select = l_conn.prepare(
      sqlpp::select(l_table.id).from(l_table).where(l_table.entity_id == sqlpp::parameter(l_table.entity_id))
  );
  for (auto& l_h : l_handles) {
    l_pre_select.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    for (auto&& row : l_conn(l_pre_select)) {
      path_handle.emplace_back(row.id);
      path_map.emplace(l_h, row.id.value());
      DOODLE_LOG_INFO(
          "选择数据库id {} -> 实体 {} 组件 {} ", row.id, l_h.entity(),
          rttr::type::get<redirection_path_info>().get_name()
      );
    }
  }

  BOOST_ASSERT(l_handles.size() == path_handle.size());

  detail::sql_com_destroy_parent_id<tables::rpi_search_path>(in_ptr, path_handle);

  auto l_path_pre =
      l_conn.prepare(sqlpp::insert_into(l_path_table)
                         .set(
                             l_path_table.parent_id        = sqlpp::parameter(l_path_table.parent_id),
                             l_path_table.redirection_path = sqlpp::parameter(l_path_table.redirection_path)
                         ));
  for (auto& l_h : l_handles) {
    auto& l_r_p_i = l_h.get<doodle::redirection_path_info>();
    for (auto& l_p : l_r_p_i.search_path_) {
      l_path_pre.params.parent_id        = path_map[l_h];
      l_path_pre.params.redirection_path = l_p.generic_string();
    }
    auto l_r_p = l_conn(l_path_pre);
    DOODLE_LOG_INFO(
        "插入数据库id {} -> 实体 {} 组件 {} ", l_r_p, l_h.entity(),
        rttr::type::get<tables::rpi_search_path>().get_name()
    );
  }
}

void sql_com<doodle::redirection_path_info>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle
) {
  auto& l_conn = *in_ptr;
  tables::redirection_path_info l_table{};
  tables::rpi_search_path l_path_table{};
  std::vector<redirection_path_info> l_r_p_i{};
  std::vector<FSys::path> l_path{};
  std::vector<entt::entity> l_entts{};
  std::map<entt::entity, std::int64_t> path_map{};
  // 调整内存

  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_r_p_i.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }
  for (auto& row : l_conn(sqlpp::select(l_table.id, l_table.entity_id, l_table.redirection_file_name)
                              .from(l_table)
                              .where(l_table.entity_id.is_null()))) {
    doodle::redirection_path_info l_r{};
    l_r.search_path_ = l_path;
    l_r.file_name_   = row.redirection_file_name.value();
    auto l_id        = row.entity_id.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_r_p_i.emplace_back(std::move(l_r));
      l_entts.emplace_back(in_handle.at(l_id));
      path_map.emplace(in_handle.at(l_id), row.id);
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  auto l_pre_select = l_conn.prepare(sqlpp::select(l_path_table.redirection_path)
                                         .from(l_path_table)
                                         .where(l_path_table.parent_id == sqlpp::parameter(l_path_table.parent_id)));

  for (auto i = 0; i < l_r_p_i.size(); ++i) {
    l_pre_select.params.parent_id = path_map[l_entts[i]];
    for (auto&& row : l_conn(l_pre_select)) {
      l_r_p_i[i].search_path_.emplace_back(row.redirection_path.value());
    }
  }
  reg_->insert(l_entts.begin(), l_entts.end(), l_r_p_i.begin());
}
void sql_com<doodle::redirection_path_info>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::redirection_path_info>(in_ptr, in_handle);
  detail::sql_com_destroy_parent_id<tables::rpi_search_path>(in_ptr, in_handle);
}
}  // namespace doodle::database_n