#include "export_file_info.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/generate/core/sql_sql.h>
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
namespace sql = doodle_database;
void sql_com<doodle::export_file_info>::insert(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_id) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_id | ranges::views::transform([&](std::int64_t in_entity) {
                     return entt::handle{*reg_, num_to_enum<entt::entity>(in_entity)};
                   }) |
                   ranges::to_vector;
  sql::ExportFileInfo l_table{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.filePath = sqlpp::parameter(l_table.filePath), l_table.startFrame = sqlpp::parameter(l_table.startFrame),
      l_table.endFrame = sqlpp::parameter(l_table.endFrame), l_table.refFile = sqlpp::parameter(l_table.refFile),
      l_table.exportType_ = sqlpp::parameter(l_table.exportType_), l_table.entityId = sqlpp::parameter(l_table.entityId)
  ));

  for (auto& l_h : l_handles) {
    auto& l_file             = l_h.get<export_file_info>();
    l_pre.params.filePath    = l_file.file_path.string();
    l_pre.params.startFrame  = l_file.start_frame;
    l_pre.params.endFrame    = l_file.end_frame;
    l_pre.params.refFile     = l_file.ref_file.string();
    l_pre.params.exportType_ = std::string{magic_enum::enum_name(l_file.export_type_)};
    l_pre.params.entityId    = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r                 = l_conn(l_pre);
    DOODLE_LOG_INFO(
        "插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<export_file_info>().get_name()
    );
  }
}

void sql_com<doodle::export_file_info>::update(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_id) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_id | ranges::views::transform([&](std::int64_t in_entity) {
                     return entt::handle{*reg_, num_to_enum<entt::entity>(in_entity)};
                   }) |
                   ranges::to_vector;
  sql::ExportFileInfo l_table{};

  auto l_pre = l_conn.prepare(sqlpp::update(l_table)
                                  .set(
                                      l_table.filePath    = sqlpp::parameter(l_table.filePath),
                                      l_table.startFrame  = sqlpp::parameter(l_table.startFrame),
                                      l_table.endFrame    = sqlpp::parameter(l_table.endFrame),
                                      l_table.refFile     = sqlpp::parameter(l_table.refFile),
                                      l_table.exportType_ = sqlpp::parameter(l_table.exportType_)
                                  )
                                  .where(l_table.entityId == sqlpp::parameter(l_table.entityId)));
  for (auto& l_h : l_handles) {
    auto& l_file             = l_h.get<export_file_info>();
    l_pre.params.filePath    = l_file.file_path.string();
    l_pre.params.startFrame  = l_file.start_frame;
    l_pre.params.endFrame    = l_file.end_frame;
    l_pre.params.refFile     = l_file.ref_file.string();
    l_pre.params.exportType_ = std::string{magic_enum::enum_name(l_file.export_type_)};
    l_pre.params.entityId    = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    auto l_r                 = l_conn(l_pre);
    DOODLE_LOG_INFO(
        "更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<export_file_info>().get_name()
    );
  }
}
void sql_com<doodle::export_file_info>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle
) {
  auto& l_conn = *in_ptr;
  sql::ExportFileInfo l_table{};
  std::vector<export_file_info> l_file;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entityId)).from(l_table).where(l_table.entityId.is_not_null()))) {
    l_file.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row : l_conn(sqlpp::select(
                              l_table.entityId, l_table.filePath, l_table.startFrame, l_table.endFrame, l_table.refFile,
                              l_table.exportType_
       )
                              .from(l_table)
                              .where(l_table.entityId.is_null()))) {
    export_file_info l_f{};
    l_f.file_path    = row.filePath.value();
    l_f.start_frame  = row.startFrame.value();
    l_f.end_frame    = row.endFrame.value();
    l_f.ref_file     = row.refFile.value();
    l_f.export_type_ = magic_enum::enum_cast<export_file_info::export_type>(row.exportType_.value())
                           .value_or(export_file_info::export_type::none);
    auto l_id = row.entityId.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_file.emplace_back(std::move(l_f));
      l_entts.emplace_back(in_handle.at(l_id));
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  reg_->insert(l_entts.begin(), l_entts.end(), l_file.begin());
}
void sql_com<doodle::export_file_info>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<sql::ExportFileInfo>(in_ptr, in_handle);
}

}  // namespace doodle::database_n