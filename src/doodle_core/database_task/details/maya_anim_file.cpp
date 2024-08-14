//
// Created by TD on 2023/12/18.
//

#include "maya_anim_file.h"

#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/maya_anim_file.h>
#include <doodle_core/metadata/metadata.h>

#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
namespace doodle::database_n {
void sql_com<doodle::maya_anim_file>::create_table(conn_ptr& in_ptr) {
  sql_create_table_base::create_table(in_ptr);

  create_table_parent_id<tables::maya_anim_file_ref_file>(in_ptr);
}

void sql_com<doodle::maya_anim_file>::install_sub(
    conn_ptr& in_ptr, const std::vector<entt::handle>& in_handles, const std::map<entt::handle, std::int64_t>& in_map
) {
  auto& l_conn = *in_ptr;
  const tables::maya_anim_file_ref_file l_table{};
  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.parent_id = sqlpp::parameter(l_table.parent_id), l_table.ref_file_ = sqlpp::parameter(l_table.ref_file_),
      l_table.path_ = sqlpp::parameter(l_table.path_)
  ));
  for (auto&& l_h : in_handles) {
    auto& l_maya_anim_file = l_h.get<maya_anim_file>();
    l_pre.params.parent_id = in_map.at(l_h);
    for (auto&& [l_ref, l_path] : l_maya_anim_file.maya_rig_file_) {
      l_pre.params.ref_file_ = boost::numeric_cast<std::int64_t>(l_ref.get<database>().get_id());
      l_pre.params.path_     = l_path.string();
      l_conn(l_pre);
    }
  }
}

void sql_com<doodle::maya_anim_file>::insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  std::map<entt::handle, std::int64_t> map_id{};
  {
    const tables::maya_anim_file l_table{};
    auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.entity_id    = sqlpp::parameter(l_table.entity_id),
        l_table.begin_frame_ = sqlpp::parameter(l_table.begin_frame_),
        l_table.end_frame_   = sqlpp::parameter(l_table.end_frame_),
        l_table.camera_path_ = sqlpp::parameter(l_table.camera_path_)
    ));
    for (auto&& i : in_id) {
      auto& l_maya_anim_file    = i.get<maya_anim_file>();
      l_pre.params.entity_id    = boost::numeric_cast<std::int64_t>(i.get<database>().get_id());
      l_pre.params.begin_frame_ = l_maya_anim_file.begin_frame_;
      l_pre.params.end_frame_   = l_maya_anim_file.end_frame_;
      l_pre.params.camera_path_ = l_maya_anim_file.camera_path_.string();
      auto l_r                  = l_conn(l_pre);
      map_id.emplace(i, l_r);
    }
  }
  install_sub(in_ptr, in_id, map_id);
}

void sql_com<doodle::maya_anim_file>::update(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  const tables::maya_anim_file l_table{};
  auto l_pre = l_conn.prepare(
      sqlpp::update(l_table)
          .set(
              l_table.begin_frame_ = sqlpp::parameter(l_table.begin_frame_),
              l_table.end_frame_   = sqlpp::parameter(l_table.end_frame_),
              l_table.camera_path_ = sqlpp::parameter(l_table.camera_path_)
          )
          .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id) && l_table.id == sqlpp::parameter(l_table.id))
  );

  for (auto& [id, l_h] : in_id) {
    auto& l_maya_anim_file    = l_h.get<maya_anim_file>();
    l_pre.params.begin_frame_ = l_maya_anim_file.begin_frame_;
    l_pre.params.end_frame_   = l_maya_anim_file.end_frame_;
    l_pre.params.camera_path_ = l_maya_anim_file.camera_path_.string();
    l_pre.params.entity_id    = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_pre.params.id           = id;

    auto l_r                  = l_conn(l_pre);
  }

  auto l_handles = in_id | ranges::views::values | ranges::to<std::vector<entt::handle>>;
  auto l_map_id  = in_id | ranges::views::transform([](auto&& l_pair) -> std::pair<entt::handle, std::int64_t> {
                    return {l_pair.second, l_pair.first};
                  }) |
                  ranges::to<std::map<entt::handle, std::int64_t>>;
  detail::sql_com_destroy_parent_id<tables::maya_anim_file_ref_file>(in_ptr, l_map_id);
  install_sub(in_ptr, l_handles, l_map_id);
}

void sql_com<doodle::maya_anim_file>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, entt::registry& in_reg
) {
  auto& l_conn = *in_ptr;

  const tables::maya_anim_file l_table{};
  std::vector<maya_anim_file> l_maya_anim_files{};
  std::vector<entt::entity> l_entts{};
  std::map<entt::entity, std::int64_t> l_map_patent{};
  {  // 调整内存
    for (auto&& raw :
         l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
      l_maya_anim_files.reserve(raw.count.value());
      l_entts.reserve(raw.count.value());
      break;
    }

    for (auto&& row :
         l_conn(sqlpp::select(
                    l_table.id, l_table.entity_id, l_table.begin_frame_, l_table.end_frame_, l_table.camera_path_
         )
                    .from(l_table)
                    .where(l_table.entity_id.is_not_null()))) {
      maya_anim_file l_maya_anim_file{};
      l_maya_anim_file.begin_frame_ = row.begin_frame_.value();
      l_maya_anim_file.end_frame_   = row.end_frame_.value();
      l_maya_anim_file.camera_path_ = row.camera_path_.value();
      auto l_id                     = row.entity_id.value();
      if (in_handle.contains(l_id)) {
        l_maya_anim_files.emplace_back(std::move(l_maya_anim_file));
        l_entts.emplace_back(in_handle.at(l_id));
        l_map_patent.insert({in_handle.at(l_id), row.id.value()});
      } else
        default_logger_raw()->log(log_loc(), spdlog::level::warn, "选择数据库id {} 未找到实体", l_id);
    }
  }

  {
    const tables::maya_anim_file_ref_file l_table{};
    auto l_pre = l_conn.prepare(sqlpp::select(l_table.path_, l_table.ref_file_)
                                    .from(l_table)
                                    .where(l_table.parent_id == sqlpp::parameter(l_table.parent_id)));
    for (auto i = 0; i < l_maya_anim_files.size(); ++i) {
      l_pre.params.parent_id = l_map_patent.at(l_entts[i]);
      for (auto&& row : l_conn(l_pre)) {
        auto l_ref_id = row.ref_file_.value();
        if (in_handle.contains(l_ref_id)) {
          l_maya_anim_files[i].maya_rig_file_.emplace_back(in_handle.at(l_ref_id), row.path_.value());
        }
      }
    }
  }
  in_reg.insert(l_entts.begin(), l_entts.end(), l_maya_anim_files.begin());
}

void sql_com<doodle::maya_anim_file>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::maya_anim_file>(in_ptr, in_handle);
  detail::sql_com_destroy_parent_id<tables::maya_anim_file_ref_file>(in_ptr, in_handle);
}
}  // namespace doodle::database_n