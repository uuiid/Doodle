#include "export_file_info.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>

#include "metadata/export_file_info.h"
#include "metadata/metadata.h"
#include <algorithm>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/parameter.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <string>
#include <vector>

namespace doodle::database_n {
void sql_com<doodle::export_file_info>::insert(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  auto& l_conn   = *in_ptr;
  auto l_handles = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  tables::export_file_info l_table{};
  auto l_pre = l_conn.prepare(sqlpp::sqlite3::insert_or_replace_into(l_table).set(
      l_table.file_path   = sqlpp::parameter(l_table.file_path),
      l_table.start_frame = sqlpp::parameter(l_table.start_frame),
      l_table.end_frame = sqlpp::parameter(l_table.end_frame), l_table.ref_file = sqlpp::parameter(l_table.ref_file),
      l_table.export_type_ = sqlpp::parameter(l_table.export_type_),
      l_table.entity_id    = sqlpp::parameter(l_table.entity_id)
  ));

  for (auto& l_h : l_handles) {
    auto& l_file              = l_h.get<export_file_info>();
    l_pre.params.file_path    = l_file.file_path.string();
    l_pre.params.start_frame  = l_file.start_frame;
    l_pre.params.end_frame    = l_file.end_frame;
    l_pre.params.ref_file     = l_file.ref_file.string();
    l_pre.params.export_type_ = std::string{magic_enum::enum_name(l_file.export_type_)};
    l_pre.params.entity_id    = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r                  = l_conn(l_pre);
    DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<export_file_info>().name());
  }
}

void sql_com<doodle::export_file_info>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle
) {
  auto& l_conn = *in_ptr;
  tables::export_file_info l_table{};
  std::vector<export_file_info> l_file;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_file.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row : l_conn(sqlpp::select(
                              l_table.entity_id, l_table.file_path, l_table.start_frame, l_table.end_frame,
                              l_table.ref_file, l_table.export_type_
       )
                              .from(l_table)
                              .where(l_table.entity_id.is_null()))) {
    export_file_info l_f{};
    l_f.file_path    = row.file_path.value();
    l_f.start_frame  = row.start_frame.value();
    l_f.end_frame    = row.end_frame.value();
    l_f.ref_file     = row.ref_file.value();
    l_f.export_type_ = magic_enum::enum_cast<export_file_info::export_type>(row.export_type_.value())
                           .value_or(export_file_info::export_type::none);
    auto l_id = row.entity_id.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_file.emplace_back(std::move(l_f));
      l_entts.emplace_back(in_handle.at(l_id));
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  reg_->insert<doodle::export_file_info>(l_entts.begin(), l_entts.end(), l_file.begin());
}
void sql_com<doodle::export_file_info>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::export_file_info>(in_ptr, in_handle);
}

}  // namespace doodle::database_n