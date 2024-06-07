#include "attendance.h"

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
DOODLE_SQL_COLUMN_IMP(remark, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(attendance_type, sqlpp::integer, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(user_ref_id, sqlpp::integer, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(update_time, sqlpp::time_point, database_n::detail::can_be_null);

DOODLE_SQL_TABLE_IMP(
    attendance_tab, tables::column::id, uuid, start_time, end_time, remark, attendance_type, update_time, user_ref_id
);
}  // namespace

std::vector<attendance> attendance::select_all(
    pooled_connection& in_comm, const std::map<boost::uuids::uuid, entt::entity>& in_map_id
) {
  auto l_get_user = user::select_all_map_id(in_comm);
  attendance_tab l_tab{};
  std::vector<attendance> l_ret{};
  auto l_pre = in_comm.prepare(sqlpp::select(all_of(l_tab)).from(l_tab).unconditionally());
  for (const auto& l_row : in_comm(l_pre)) {
    attendance l_tmp{};
    std::copy_n(l_row.uuid.value().begin(), l_tmp.id_.size(), l_tmp.id_.begin());
    l_tmp.start_time_  = chrono::zoned_time<chrono::microseconds>{chrono::current_zone(), l_row.start_time.value()};
    l_tmp.end_time_    = chrono::zoned_time<chrono::microseconds>{chrono::current_zone(), l_row.end_time.value()};
    l_tmp.remark_      = l_row.remark;
    l_tmp.type_        = static_cast<att_enum>(l_row.attendance_type.value());
    l_tmp.update_time_ = chrono::zoned_time<chrono::microseconds>{chrono::current_zone(), l_row.update_time.value()};

    if (l_get_user.contains(l_row.user_ref_id.value()) == false) {
      continue;
    }
    if (in_map_id.contains(l_get_user.at(l_row.user_ref_id.value())) == false) {
      continue;
    }
    l_tmp.user_ref_id_ = in_map_id.at(l_get_user.at(l_row.user_ref_id.value()));
    l_ret.emplace_back(std::move(l_tmp));
  }
  return l_ret;
}
void attendance::create_table(pooled_connection& in_comm) {
  // 外键 user_ref_id-> (user) id
  in_comm.execute(R"(
    CREATE TABLE IF NOT EXISTS attendance_tab (
      id                INTEGER PRIMARY KEY AUTOINCREMENT,
      uuid              BLOB NOT NULL    UNIQUE,
      start_time        TIMESTAMP NOT NULL,
      end_time          TIMESTAMP NOT NULL,
      remark            TEXT,
      attendance_type   INTEGER NOT NULL,
      update_time       TIMESTAMP,
      user_ref_id       INTEGER REFERENCES user_tab(id) ON DELETE CASCADE
    );
  )");
  in_comm.execute(R"(
    CREATE INDEX IF NOT EXISTS attendance_tab_uuid_index ON attendance_tab (uuid);
  )");
}

std::vector<bool> attendance::filter_exist(pooled_connection& in_comm, const std::vector<attendance>& in_task) {
  attendance_tab l_tab{};
  std::vector<bool> l_ret{};
  auto l_pre = in_comm.prepare(
      sqlpp::select(sqlpp::count(l_tab.id)).from(l_tab).where(l_tab.uuid == sqlpp::parameter(l_tab.uuid))
  );
  for (const auto& l_attendance : in_task) {
    l_pre.params.uuid = {l_attendance.id_.begin(), l_attendance.id_.end()};
    for (const auto& l_row : in_comm(l_pre)) {
      l_ret.emplace_back(l_row.count > 0);
      break;
    }
  }
  return l_ret;
}
void attendance::insert(
    pooled_connection& in_comm, const std::vector<attendance>& in_task,
    const std::map<entt::entity, boost::uuids::uuid>& in_map_id
) {
  auto l_get_user    = user::select_all_map_id(in_comm);
  // 反转key 和value
  auto l_get_user_id = [&l_get_user]() -> std::map<boost::uuids::uuid, std::int64_t> {
    std::map<boost::uuids::uuid, std::int64_t> l_ret{};
    for (const auto& [l_key, l_value] : l_get_user) {
      l_ret[l_value] = l_key;
    }
    return l_ret;
  }();

  attendance_tab l_tab{};
  auto l_pre = in_comm.prepare(sqlpp::insert_into(l_tab).set(
      l_tab.uuid = sqlpp::parameter(l_tab.uuid), l_tab.start_time = sqlpp::parameter(l_tab.start_time),
      l_tab.end_time = sqlpp::parameter(l_tab.end_time), l_tab.remark = sqlpp::parameter(l_tab.remark),
      l_tab.attendance_type = sqlpp::parameter(l_tab.attendance_type),
      l_tab.update_time = sqlpp::parameter(l_tab.update_time), l_tab.user_ref_id = sqlpp::parameter(l_tab.user_ref_id)
  ));
  for (const auto& l_attendance : in_task) {
    l_pre.params.uuid            = {l_attendance.id_.begin(), l_attendance.id_.end()};
    l_pre.params.start_time      = l_attendance.start_time_.get_sys_time();
    l_pre.params.end_time        = l_attendance.end_time_.get_sys_time();
    l_pre.params.remark          = l_attendance.remark_;
    l_pre.params.attendance_type = static_cast<std::uint32_t>(l_attendance.type_);
    l_pre.params.update_time     = l_attendance.update_time_.get_sys_time();
    if (in_map_id.contains(l_attendance.user_ref_id_) == false) {
      continue;
    }
    if (l_get_user_id.contains(in_map_id.at(l_attendance.user_ref_id_)) == false) {
      continue;
    }
    l_pre.params.user_ref_id = l_get_user_id.at(in_map_id.at(l_attendance.user_ref_id_));
    in_comm(l_pre);
  }
}
void attendance::update(pooled_connection& in_comm, const std::vector<attendance>& in_task) {
  attendance_tab l_tab{};
  auto l_pre = in_comm.prepare(
      sqlpp::update(l_tab)
          .set(
              l_tab.start_time = sqlpp::parameter(l_tab.start_time), l_tab.end_time = sqlpp::parameter(l_tab.end_time),
              l_tab.remark = sqlpp::parameter(l_tab.remark), l_tab.update_time = sqlpp::parameter(l_tab.update_time),
              l_tab.attendance_type = sqlpp::parameter(l_tab.attendance_type)
          )
          .where(l_tab.uuid == sqlpp::parameter(l_tab.uuid))
  );
  for (const auto& l_attendance : in_task) {
    l_pre.params.start_time      = l_attendance.start_time_.get_sys_time();
    l_pre.params.end_time        = l_attendance.end_time_.get_sys_time();
    l_pre.params.remark          = l_attendance.remark_;
    l_pre.params.attendance_type = static_cast<std::uint32_t>(l_attendance.type_);
    l_pre.params.uuid            = {l_attendance.id_.begin(), l_attendance.id_.end()};
    l_pre.params.update_time     = l_attendance.update_time_.get_sys_time();
    in_comm(l_pre);
  }
}
void attendance::delete_by_ids(pooled_connection& in_comm, const std::vector<boost::uuids::uuid>& in_ids) {
  attendance_tab l_tab{};
  auto l_pre = in_comm.prepare(sqlpp::remove_from(l_tab).where(l_tab.uuid == sqlpp::parameter(l_tab.uuid)));
  for (const auto& l_id : in_ids) {
    l_pre.params.uuid = {l_id.begin(), l_id.end()};
    in_comm(l_pre);
  }
}

void to_json(nlohmann::json& j, const attendance& p) {
  j["id"]          = fmt::to_string(p.id_);
  j["start_time"]  = fmt::format("%FT%T", p.start_time_.get_local_time());
  j["end_time"]    = fmt::format("%FT%T", p.end_time_.get_local_time());
  j["remark"]      = p.remark_;
  j["update_time"] = fmt::format("%FT%T", p.update_time_.get_local_time());
  j["type"]        = static_cast<std::uint32_t>(p.type_);
}
// void from_json(const nlohmann::json& j, attendance& p) {
//   p.id_          = j.at("id").get<boost::uuids::uuid>();
//   p.start_time_  = j.at("start_time").get<chrono::zoned_time<chrono::microseconds>>();
//   p.end_time_    = j.at("end_time").get<chrono::zoned_time<chrono::microseconds>>();
//   p.remark_      = j.at("remark").get<std::string>();
//   p.type_        = static_cast<attendance_type_enum>(j.at("type").get<std::uint32_t>());
//   p.user_ref_id_ = j.at("user_ref_id").get<entt::entity>();
// }

}  // namespace doodle