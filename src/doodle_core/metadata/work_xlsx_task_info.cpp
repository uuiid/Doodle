#include "work_xlsx_task_info.h"

#include <doodle_core/database_task/details/column.h>
#include <doodle_core/lib_warp/sqlite3/sqlite3.h>

#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
//
#include "doodle_core/metadata/user.h"
namespace doodle {

namespace {
DOODLE_SQL_COLUMN_IMP(uuid, sqlpp::blob, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(start_time, sqlpp::time_point, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(end_time, sqlpp::time_point, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(year_c, sqlpp::integer, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(month_c, sqlpp::integer, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(duration, sqlpp::integer, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(kitsu_task_ref_id, sqlpp::blob, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(sort_id_, sqlpp::integer, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(user_ref_id, sqlpp::integer, database_n::detail::can_be_null);

DOODLE_SQL_TABLE_IMP(
    work_xlsx_task_info_tab, tables::column::id, uuid, start_time, end_time, year_c, month_c, duration,
    kitsu_task_ref_id, sort_id_, user_ref_id
);
}  // namespace

std::vector<work_xlsx_task_info> work_xlsx_task_info::select_all(
    pooled_connection& in_comm, const std::map<boost::uuids::uuid, entt::entity>& in_map_id
) {
  auto l_user_id_maps = user::select_all_map_id(in_comm);
  work_xlsx_task_info_tab l_tab{};

  std::vector<work_xlsx_task_info> l_ret{};
  std::vector<std::int64_t> l_ret_id{};
  std::map<std::int64_t, std::size_t> l_map_id{};
  for (auto&& l_row : in_comm(sqlpp::select(sqlpp::all_of(l_tab)).from(l_tab).unconditionally())) {
    work_xlsx_task_info l_info{};
    std::copy_n(l_row.uuid.value().begin(), l_info.id_.size(), l_info.id_.begin());
    l_info.year_month_ = chrono::year_month{
        chrono::year{boost::numeric_cast<std::int32_t>(l_row.year_c.value())},
        chrono::month{boost::numeric_cast<std::uint32_t>(l_row.month_c.value())}
    };
    l_info.start_time_ = l_row.start_time.value();
    l_info.end_time_   = l_row.end_time.value();
    l_info.duration_   = chrono::system_clock::duration{l_row.duration.value()};
    l_info.sort_id_    = l_row.sort_id_.value();
    if (!l_row.kitsu_task_ref_id.is_null())
      std::copy_n(
          l_row.kitsu_task_ref_id.value().begin(), l_info.kitsu_task_ref_id_.size(), l_info.kitsu_task_ref_id_.begin()
      );
    if (!l_user_id_maps.contains(l_row.user_ref_id.value())) continue;
    l_info.user_refs_ = in_map_id.at(l_user_id_maps.at(l_row.user_ref_id.value()));
  }

  return l_ret;
}
void work_xlsx_task_info::create_table(pooled_connection& in_comm) {
  in_comm.execute(R"(
    CREATE TABLE IF NOT EXISTS work_xlsx_task_info_tab (
      id                INTEGER    PRIMARY KEY NOT NULL,
      uuid              BLOB       NOT NULL UNIQUE,
      start_time        TIMESTAMP,
      end_time          TIMESTAMP,
      year_c            INTEGER,
      month_c           INTEGER,
      duration          INTEGER,
      kitsu_task_ref_id BLOB,
      sort_id_          INTEGER,
      user_ref_id       INTEGER     REFERENCES user_tab(id) ON DELETE CASCADE
    );
  )");
  in_comm.execute(R"(
    CREATE INDEX IF NOT EXISTS work_xlsx_task_info_tab_uuid_index ON work_xlsx_task_info_tab (uuid);
  )");
}
std::vector<bool> work_xlsx_task_info::filter_exist(
    pooled_connection& in_comm, const std::vector<work_xlsx_task_info>& in_task
) {
  work_xlsx_task_info_tab l_tab{};
  std::vector<bool> l_ret{};
  auto l_pre = in_comm.prepare(
      sqlpp::select(sqlpp::count(l_tab.id)).from(l_tab).where(l_tab.uuid == sqlpp::parameter(l_tab.uuid))
  );
  for (const auto& l_info : in_task) {
    l_pre.params.uuid = {l_info.id_.begin(), l_info.id_.end()};
    for (const auto& l_row : in_comm(l_pre)) {
      l_ret.emplace_back(l_row.count.value() > 0);
      break;
    }
  }
  return l_ret;
}
void work_xlsx_task_info::insert(
    pooled_connection& in_comm, const std::vector<work_xlsx_task_info>& in_task,
    const std::map<entt::entity, boost::uuids::uuid>& in_map_id
) {
  auto l_user_id_maps = user::select_all_map_id(in_comm);
  // 反转key value
  std::map<boost::uuids::uuid, std::int64_t> l_user_uuid_maps{};
  for (const auto& [l_id, l_uuid] : l_user_id_maps) {
    l_user_uuid_maps[l_uuid] = l_id;
  }

  work_xlsx_task_info_tab l_tab{};
  auto l_pre = in_comm.prepare(sqlpp::insert_into(l_tab).set(
      l_tab.uuid = sqlpp::parameter(l_tab.uuid), l_tab.start_time = sqlpp::parameter(l_tab.start_time),
      l_tab.end_time = sqlpp::parameter(l_tab.end_time), l_tab.year_c = sqlpp::parameter(l_tab.year_c),
      l_tab.month_c = sqlpp::parameter(l_tab.month_c), l_tab.duration = sqlpp::parameter(l_tab.duration),
      l_tab.kitsu_task_ref_id = sqlpp::parameter(l_tab.kitsu_task_ref_id),
      l_tab.sort_id_ = sqlpp::parameter(l_tab.sort_id_), l_tab.user_ref_id = sqlpp::parameter(l_tab.user_ref_id)
  ));
  for (const auto& l_info : in_task) {
    l_pre.params.uuid              = {l_info.id_.begin(), l_info.id_.end()};
    l_pre.params.start_time        = chrono::time_point_cast<chrono::microseconds>(l_info.start_time_);
    l_pre.params.end_time          = chrono::time_point_cast<chrono::microseconds>(l_info.end_time_);
    l_pre.params.year_c            = std::int32_t(l_info.year_month_.year());
    l_pre.params.month_c           = std::uint32_t(l_info.year_month_.month());
    l_pre.params.duration          = l_info.duration_.count();
    l_pre.params.sort_id_          = l_info.sort_id_;
    l_pre.params.kitsu_task_ref_id = {l_info.kitsu_task_ref_id_.begin(), l_info.kitsu_task_ref_id_.end()};
    l_pre.params.user_ref_id       = l_user_uuid_maps.at(in_map_id.at(l_info.user_refs_));
    in_comm(l_pre);
  }
}
void work_xlsx_task_info::update(pooled_connection& in_comm, const std::vector<work_xlsx_task_info>& in_task) {
  work_xlsx_task_info_tab l_tab{};
  auto l_pre = in_comm.prepare(
      sqlpp::update(l_tab)
          .set(
              l_tab.start_time = sqlpp::parameter(l_tab.start_time), l_tab.end_time = sqlpp::parameter(l_tab.end_time),
              l_tab.year_c = sqlpp::parameter(l_tab.year_c), l_tab.month_c = sqlpp::parameter(l_tab.month_c),
              l_tab.duration = sqlpp::parameter(l_tab.duration), l_tab.sort_id_ = sqlpp::parameter(l_tab.sort_id_),
              l_tab.kitsu_task_ref_id = sqlpp::parameter(l_tab.kitsu_task_ref_id)
          )
          .where(l_tab.uuid == sqlpp::parameter(l_tab.uuid))
  );
  for (const auto& l_info : in_task) {
    l_pre.params.start_time        = chrono::time_point_cast<chrono::microseconds>(l_info.start_time_);
    l_pre.params.end_time          = chrono::time_point_cast<chrono::microseconds>(l_info.end_time_);
    l_pre.params.year_c            = std::int32_t(l_info.year_month_.year());
    l_pre.params.month_c           = std::uint32_t(l_info.year_month_.month());
    l_pre.params.duration          = l_info.duration_.count();
    l_pre.params.kitsu_task_ref_id = {l_info.kitsu_task_ref_id_.begin(), l_info.kitsu_task_ref_id_.end()};
    l_pre.params.uuid              = {l_info.id_.begin(), l_info.id_.end()};
    l_pre.params.sort_id_          = l_info.sort_id_;
    in_comm(l_pre);
  }
}
void work_xlsx_task_info::delete_by_ids(pooled_connection& in_comm, const std::vector<boost::uuids::uuid>& in_ids) {
  work_xlsx_task_info_tab l_tab{};
  auto l_pre = in_comm.prepare(sqlpp::remove_from(l_tab).where(l_tab.uuid == sqlpp::parameter(l_tab.uuid)));
  for (const auto& l_id : in_ids) {
    l_pre.params.uuid = {l_id.begin(), l_id.end()};
    in_comm(l_pre);
  }
}

}  // namespace doodle