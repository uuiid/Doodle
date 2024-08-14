#include "assets_file.h"

#include <doodle_core/core/core_help_impl.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/metadata.h>

#include <boost/asio.hpp>
#include <boost/locale.hpp>

#include "wil/win32_helpers.h"
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/parameter.h>
#include <sqlpp11/sqlpp11.h>
#include <vector>
#include <wil/result.h>
namespace doodle::database_n {

void sql_com<doodle::assets_file>::create_table(conn_ptr& in_ptr) { sql_create_table_base::create_table(in_ptr); }

void sql_com<doodle::assets_file>::insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id) {
  auto& l_conn = *in_ptr;
  tables::assets_file const l_table{};
  sql_create_table_base::create_table(in_ptr, l_table.organization);

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.name = sqlpp::parameter(l_table.name), l_table.path = sqlpp::parameter(l_table.path),
      l_table.version = sqlpp::parameter(l_table.version), l_table.ref_id = sqlpp::parameter(l_table.ref_id),
      l_table.entity_id     = sqlpp::parameter(l_table.entity_id),
      l_table.assets_ref_id = sqlpp::parameter(l_table.assets_ref_id),
      l_table.organization  = sqlpp::parameter(l_table.organization)
  ));

  for (const auto& l_h : in_id) {
    auto& l_assets            = l_h.get<assets_file>();
    l_pre.params.name         = l_assets.name_attr();
    l_pre.params.path         = l_assets.path_attr().generic_string();
    l_pre.params.version      = l_assets.version_attr();
    l_pre.params.organization = l_assets.organization_attr();
    if (auto l_h_user = l_assets.user_attr(); l_h_user && l_h_user.any_of<database>())
      l_pre.params.ref_id = l_h_user.get<database>().get_id();
    else {
      l_pre.params.ref_id.set_null();
    }
    if (auto l_h_assets = l_assets.assets_attr(); l_h_assets && l_h_assets.any_of<database>())
      l_pre.params.assets_ref_id = l_h_assets.get<database>().get_id();
    else {
      l_pre.params.assets_ref_id.set_null();
    }
    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_conn(l_pre);
    try {
      FSys::software_flag_file(l_assets.path_attr(), l_h.get<database>().uuid());
    } catch (const wil::ResultException& e) {
      log_error(fmt::format("创建软件标记文件失败 {}", e.what()));
    }
  }
}

void sql_com<doodle::assets_file>::update(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id) {
  auto& l_conn = *in_ptr;
  tables::assets_file const l_table{};
  sql_create_table_base::create_table(in_ptr, l_table.organization);

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_table)
          .set(
              l_table.name = sqlpp::parameter(l_table.name), l_table.path = sqlpp::parameter(l_table.path),
              l_table.version = sqlpp::parameter(l_table.version), l_table.ref_id = sqlpp::parameter(l_table.ref_id),
              l_table.assets_ref_id = sqlpp::parameter(l_table.assets_ref_id),
              l_table.organization  = sqlpp::parameter(l_table.organization)
          )
          .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id) && l_table.id == sqlpp::parameter(l_table.id))
  );
  std::vector<std::pair<FSys::path, uuid>> l_path_uuid{};
  for (const auto& [id, l_h] : in_id) {
    auto& l_assets            = l_h.get<assets_file>();
    l_pre.params.id           = id;
    l_pre.params.name         = l_assets.name_attr();
    l_pre.params.path         = l_assets.path_attr().string();
    l_pre.params.version      = l_assets.version_attr();
    l_pre.params.organization = l_assets.organization_attr();

    if (auto l_h_user = l_assets.user_attr(); l_h_user && l_h_user.any_of<database>())
      l_pre.params.ref_id = l_h_user.get<database>().get_id();
    else {
      l_pre.params.ref_id.set_null();
    }

    if (auto l_h_assets = l_assets.assets_attr(); l_h_assets && l_h_assets.any_of<database>())
      l_pre.params.assets_ref_id = l_h_assets.get<database>().get_id();
    else {
      l_pre.params.assets_ref_id.set_null();
    }

    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_conn(l_pre);
    l_path_uuid.emplace_back(l_assets.path_attr(), l_h.get<database>().uuid());
  }

  boost::asio::post(g_thread(), [=]() {
    for (auto [l_p, l_uuid] : l_path_uuid) {
      try {
        FSys::software_flag_file(l_p, l_uuid);
      } catch (const wil::ResultException& e) {
        default_logger_raw()->log(log_loc(), spdlog::level::warn, "创建软件标记文件失败 {}", e.what());
      } catch (const FSys::filesystem_error& e) {
        default_logger_raw()->log(log_loc(), spdlog::level::warn, "创建软件标记文件失败 {}", e.what());
      }
    }
  });
}
void sql_com<doodle::assets_file>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, entt::registry& in_reg
) {
  auto& l_conn = *in_ptr;
  const tables::assets_file l_table{};
  std::vector<assets_file> l_assets;
  std::vector<entt::entity> l_entts;
  std::map<entt::entity, std::size_t> l_map_id{};
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_assets.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  static boost::locale::generator l_gen{};
  auto l_utf8 = l_gen("zh_CN.UTF-8");
  {
    std::size_t l_index{};
    for (const auto& row : l_conn(sqlpp::select(
                                      l_table.entity_id, l_table.name, l_table.path, l_table.version, l_table.ref_id,
                                      l_table.assets_ref_id
         )
                                      .from(l_table)
                                      .where(l_table.entity_id.is_not_null())
                                      .order_by(l_table.id.asc()))) {
      assets_file l_a{};
      l_a.name_attr(row.name.value());
      l_a.path_attr({row.path.value(), l_utf8});
      l_a.version_attr(row.version.value());
      if (!row.ref_id.is_null() && in_handle.contains(row.ref_id.value()))
        l_a.user_ref.handle_cache = in_handle.at(row.ref_id.value());
      if (!row.assets_ref_id.is_null() && in_handle.contains(row.assets_ref_id.value()))
        l_a.assets_attr(in_handle.at(row.assets_ref_id.value()));

      auto l_id = row.entity_id.value();
      if (in_handle.contains(l_id)) {
        l_assets.emplace_back(std::move(l_a));
        l_entts.emplace_back(in_handle.at(l_id));
        l_map_id.emplace(in_handle.at(l_id), l_index++);
        // DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
      } else {
        // DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
      }
    }
  }

  if (has_colume(in_ptr, l_table.organization)) {
    for (const auto& row : l_conn(sqlpp::select(l_table.entity_id, l_table.organization)
                                      .from(l_table)
                                      .where(l_table.entity_id.is_not_null())
                                      .order_by(l_table.id.asc()))) {
      auto l_id = row.entity_id.value();
      if (in_handle.find(l_id) != in_handle.end()) {
        l_assets[l_map_id.at(in_handle.at(l_id))].organization_attr(row.organization.value());
      }
    }
  }

  in_reg.insert<doodle::assets_file>(l_entts.begin(), l_entts.end(), l_assets.begin());
}
void sql_com<doodle::assets_file>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::assets_file>(in_ptr, in_handle);
}
}  // namespace doodle::database_n