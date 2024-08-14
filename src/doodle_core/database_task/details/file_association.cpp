//
// Created by td_main on 2023/11/9.
//

#include "file_association.h"
namespace doodle::database_n {
void sql_com<doodle::file_association>::insert(doodle::conn_ptr &in_ptr, const std::vector<entt::handle> &in_id) {
  auto &l_conn = *in_ptr;

  tables::file_association l_table{};
  sql_create_table_base::create_table(in_ptr, l_table.entity_maya_sim_file);

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.entity_id             = sqlpp::parameter(l_table.entity_id),
      l_table.entity_maya_file      = sqlpp::parameter(l_table.entity_maya_file),
      l_table.entity_maya_sim_file  = sqlpp::parameter(l_table.entity_maya_sim_file),
      l_table.entity_maya_rig_file  = sqlpp::parameter(l_table.entity_maya_rig_file),
      l_table.entity_ue_file        = sqlpp::parameter(l_table.entity_ue_file),
      l_table.entity_ue_preset_file = sqlpp::parameter(l_table.entity_ue_preset_file),
      l_table.name                  = sqlpp::parameter(l_table.name)
  ));

  for (auto &l_h : in_id) {
    auto &l_file_ass = l_h.get<file_association>();
    if (l_file_ass.maya_file && l_file_ass.maya_file.any_of<database>())
      l_pre.params.entity_maya_file = l_file_ass.maya_file.get<database>().get_id();
    if (l_file_ass.maya_rig_file && l_file_ass.maya_rig_file.any_of<database>())
      l_pre.params.entity_maya_rig_file = l_file_ass.maya_rig_file.get<database>().get_id();
    if (l_file_ass.ue_file && l_file_ass.ue_file.any_of<database>())
      l_pre.params.entity_ue_file = l_file_ass.ue_file.get<database>().get_id();
    if (l_file_ass.ue_preset_file && l_file_ass.ue_preset_file.any_of<database>())
      l_pre.params.entity_ue_preset_file = l_file_ass.ue_preset_file.get<database>().get_id();
    if (l_file_ass.maya_sim_file && l_file_ass.maya_sim_file.any_of<database>())
      l_pre.params.entity_maya_sim_file = l_file_ass.ue_preset_file.get<database>().get_id();

    l_pre.params.name      = l_file_ass.name;
    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_conn(l_pre);
  }
}
void sql_com<doodle::file_association>::update(
    doodle::conn_ptr &in_ptr, const std::map<std::int64_t, entt::handle> &in_id
) {
  auto &l_conn = *in_ptr;

  tables::file_association l_table{};
  sql_create_table_base::create_table(in_ptr, l_table.entity_maya_sim_file);

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_table)
          .set(
              l_table.entity_maya_file      = sqlpp::parameter(l_table.entity_maya_file),
              l_table.entity_maya_rig_file  = sqlpp::parameter(l_table.entity_maya_rig_file),
              l_table.entity_ue_file        = sqlpp::parameter(l_table.entity_ue_file),
              l_table.entity_ue_preset_file = sqlpp::parameter(l_table.entity_ue_preset_file),
              l_table.entity_maya_sim_file  = sqlpp::parameter(l_table.entity_maya_sim_file),
              l_table.name                  = sqlpp::parameter(l_table.name)
          )
          .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id) && l_table.id == sqlpp::parameter(l_table.id))
  );

  for (auto &[id, l_h] : in_id) {
    auto &l_file_ass = l_h.get<file_association>();
    if (l_file_ass.maya_file && l_file_ass.maya_file.any_of<database>())
      l_pre.params.entity_maya_file = l_file_ass.maya_file.get<database>().get_id();
    if (l_file_ass.maya_rig_file && l_file_ass.maya_rig_file.any_of<database>())
      l_pre.params.entity_maya_rig_file = l_file_ass.maya_rig_file.get<database>().get_id();
    if (l_file_ass.ue_file && l_file_ass.ue_file.any_of<database>())
      l_pre.params.entity_ue_file = l_file_ass.ue_file.get<database>().get_id();
    if (l_file_ass.ue_preset_file && l_file_ass.ue_preset_file.any_of<database>())
      l_pre.params.entity_ue_preset_file = l_file_ass.ue_preset_file.get<database>().get_id();
    if (l_file_ass.maya_sim_file && l_file_ass.maya_sim_file.any_of<database>())
      l_pre.params.entity_maya_sim_file = l_file_ass.ue_preset_file.get<database>().get_id();

    l_pre.params.name      = l_file_ass.name;
    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_pre.params.id        = id;
    l_conn(l_pre);
  }
}
void sql_com<doodle::file_association>::select(
    conn_ptr &in_ptr, const std::map<std::int64_t, entt::handle> &in_handle, entt::registry &in_reg
) {
  auto &l_conn = *in_ptr;
  tables::file_association l_table{};
  std::vector<file_association> l_img{};
  std::vector<entt::entity> l_entts{};
  std::map<entt::entity, std::size_t> l_map_id{};
  // 调整内存
  for (auto &&raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_img.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  {
    std::size_t l_index{};
    for (auto &row : l_conn(sqlpp::select(
                                l_table.entity_maya_file, l_table.entity_maya_rig_file, l_table.entity_ue_file,
                                l_table.entity_ue_preset_file, l_table.name, l_table.entity_id
         )
                                .from(l_table)
                                .where(l_table.entity_id.is_not_null())
                                .order_by(l_table.id.asc()))) {
      file_association l_i{};
      if (!row.entity_maya_file.is_null() && in_handle.contains(row.entity_maya_file.value()))
        l_i.maya_file = in_handle.at(row.entity_maya_file.value());
      if (!row.entity_maya_rig_file.is_null() && in_handle.contains(row.entity_maya_rig_file.value()))
        l_i.maya_rig_file = in_handle.at(row.entity_maya_rig_file.value());
      if (!row.entity_ue_file.is_null() && in_handle.contains(row.entity_ue_file.value()))
        l_i.ue_file = in_handle.at(row.entity_ue_file.value());
      if (!row.entity_ue_preset_file.is_null() && in_handle.contains(row.entity_ue_preset_file.value()))
        l_i.ue_preset_file = in_handle.at(row.entity_ue_preset_file.value());

      l_i.name  = row.name.value();

      auto l_id = row.entity_id.value();
      if (in_handle.contains(l_id)) {
        l_img.emplace_back(std::move(l_i));
        l_entts.emplace_back(in_handle.at(l_id));
        l_map_id.emplace(in_handle.at(l_id), l_index++);
        // DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
      } else {
        // DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
      }
    }
  }

  // 添加后续表格兼容性查找
  if (has_colume(in_ptr, l_table.entity_maya_sim_file)) {
    for (const auto &row : l_conn(sqlpp::select(l_table.entity_maya_sim_file, l_table.entity_id)
                                      .from(l_table)
                                      .where(l_table.entity_id.is_not_null())
                                      .order_by(l_table.id.asc()))) {
      if (!row.entity_maya_sim_file.is_null() && in_handle.contains(row.entity_maya_sim_file.value())) {
        auto l_id = row.entity_id.value();
        if (in_handle.contains(l_id)) {
          l_img[l_map_id.at(in_handle.at(l_id))].maya_sim_file = in_handle.at(row.entity_maya_sim_file.value());
        }
      }
    }
  }

  in_reg.insert<file_association>(l_entts.begin(), l_entts.end(), l_img.begin());
}
void sql_com<doodle::file_association>::destroy(doodle::conn_ptr &in_ptr, const std::vector<std::int64_t> &in_handle) {
  detail::sql_com_destroy<tables::file_association>(in_ptr, in_handle);
}
}  // namespace doodle::database_n