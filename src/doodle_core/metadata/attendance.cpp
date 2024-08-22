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
DOODLE_SQL_COLUMN_IMP(dingding_id, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(create_date, sqlpp::date, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(parent_id, sqlpp::integer, database_n::detail::can_be_null);

DOODLE_SQL_TABLE_IMP(attendance_block_tab, tables::column::id, uuid, update_time, create_date, user_ref_id);
DOODLE_SQL_TABLE_IMP(
    attendance_block_sub_tab, tables::column::id, uuid, start_time, end_time, remark, attendance_type, dingding_id,
    parent_id
);

}  // namespace

void to_json(nlohmann::json& j, const attendance& p) {
  j["id"]          = fmt::to_string(p.id_);
  j["start_time"]  = fmt::format("{:%FT%T}", p.start_time_.get_local_time());
  j["end_time"]    = fmt::format("{:%FT%T}", p.end_time_.get_local_time());
  j["remark"]      = p.remark_;
  j["type"]        = static_cast<std::uint32_t>(p.type_);
}

std::vector<attendance_block> attendance_block::select_all(
    const sql_connection_ptr& in_comm, const std::map<boost::uuids::uuid, entt::entity>& in_map_id
) {
  auto l_get_user = user::select_all_map_id(in_comm);
  attendance_block_tab l_tab{};
  attendance_block_sub_tab l_sub_tab{};
  std::vector<attendance_block> l_ret{};
  std::map<std::int64_t, std::size_t> l_ids{};
  {
    auto l_pre = in_comm->prepare(sqlpp::select(all_of(l_tab)).from(l_tab).unconditionally());
    for (const auto& l_row : (*in_comm)(l_pre)) {
      if (l_get_user.contains(l_row.user_ref_id.value()) == false) {
        continue;
      }
      if (in_map_id.contains(l_get_user.at(l_row.user_ref_id.value())) == false) {
        continue;
      }
      boost::uuids::uuid l_uuid{};
      std::copy_n(l_row.uuid.value().begin(), l_uuid.size(), l_uuid.begin());

      l_ret.emplace_back(attendance_block{
          .id_          = l_uuid,
          .create_date_ = l_row.create_date.value(),
          .update_time_ = chrono::zoned_time<chrono::microseconds>{chrono::current_zone(), l_row.update_time.value()},
          .user_ref_id_ = in_map_id.at(l_get_user.at(l_row.user_ref_id.value()))
      });

      l_ids.emplace(l_row.id, l_ret.size() - 1);
    }
  }

  {
    std::vector<std::int64_t> l_tmp_ids{};
    std::transform(l_ids.begin(), l_ids.end(), std::back_inserter(l_tmp_ids), [](const auto& in) -> std::int64_t {
      return in.first;
    });
    for (const auto& l_row : (*in_comm)(sqlpp::select(all_of(l_sub_tab))
                                         .from(l_sub_tab)
                                         .where(l_sub_tab.parent_id.in(sqlpp::value_list(l_tmp_ids))))) {
      auto& l_b = l_ret.at(l_ids.at(l_row.parent_id));

      boost::uuids::uuid l_uuid{};
      std::copy_n(l_row.uuid.value().begin(), l_uuid.size(), l_uuid.begin());

      l_b.attendance_block_.emplace_back(attendance{
          .id_          = l_uuid,
          .start_time_  = chrono::zoned_time<chrono::microseconds>{chrono::current_zone(), l_row.start_time},
          .end_time_    = chrono::zoned_time<chrono::microseconds>{chrono::current_zone(), l_row.end_time},
          .remark_      = l_row.remark,
          .type_        = static_cast<attendance::att_enum>(l_row.attendance_type.value()),
          .dingding_id_ = l_row.dingding_id,
      });
    }
  }
  return l_ret;
}
void attendance_block::create_table(const sql_connection_ptr& in_comm) {
  // 外键 user_ref_id-> (user) id
  in_comm->execute(R"(
    CREATE TABLE IF NOT EXISTS attendance_block_tab (
      id                INTEGER PRIMARY KEY AUTOINCREMENT,
      uuid              BLOB NOT NULL    UNIQUE,
      update_time       TIMESTAMP,
      create_date       DATE,
      user_ref_id       INTEGER REFERENCES user_tab(id) ON DELETE CASCADE
    );
  )");
  in_comm->execute(R"(
    CREATE INDEX IF NOT EXISTS attendance_block_tab_uuid_index ON attendance_block_tab (uuid);
  )");

  in_comm->execute(R"(
    CREATE TABLE IF NOT EXISTS attendance_block_sub_tab (
      id                INTEGER PRIMARY KEY AUTOINCREMENT,
      uuid              BLOB NOT NULL    UNIQUE,
      start_time        TIMESTAMP NOT NULL,
      end_time          TIMESTAMP NOT NULL,
      remark            TEXT,
      attendance_type   INTEGER NOT NULL,
      dingding_id       TEXT,
      parent_id         INTEGER REFERENCES attendance_block_tab(id) ON DELETE CASCADE
    );
  )");
  in_comm->execute(R"(
    CREATE INDEX IF NOT EXISTS attendance_block_sub_tab_uuid_index ON attendance_block_sub_tab (uuid);
  )");
}

// 过滤已经存在的任务
std::vector<bool> attendance_block::filter_exist(
    const sql_connection_ptr& in_comm, const std::vector<attendance_block>& in_task
) {
  attendance_block_tab l_tab{};
  std::vector<bool> l_ret{};
  auto l_pre = in_comm->prepare(
      sqlpp::select(sqlpp::count(l_tab.id)).from(l_tab).where(l_tab.uuid == sqlpp::parameter(l_tab.uuid))
  );
  for (const auto& l_attendance_block : in_task) {
    l_pre.params.uuid = {l_attendance_block.id_.begin(), l_attendance_block.id_.end()};
    for (const auto& l_row : (*in_comm)(l_pre)) {
      l_ret.emplace_back(l_row.count > 0);
      break;
    }
  }
  return l_ret;
}
void attendance_block::insert(
    const sql_connection_ptr& in_comm, const std::vector<attendance_block>& in_task,
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

  attendance_block_tab l_tab{};
  auto l_pre = in_comm->prepare(sqlpp::insert_into(l_tab).set(
      l_tab.uuid = sqlpp::parameter(l_tab.uuid), l_tab.update_time = sqlpp::parameter(l_tab.update_time),
      l_tab.create_date = sqlpp::parameter(l_tab.create_date), l_tab.user_ref_id = sqlpp::parameter(l_tab.user_ref_id)
  ));
  std::vector<std::int64_t> l_ids{};
  for (const auto& l_attendance_block : in_task) {
    l_pre.params.uuid        = {l_attendance_block.id_.begin(), l_attendance_block.id_.end()};
    l_pre.params.update_time = l_attendance_block.update_time_.get_sys_time();
    l_pre.params.create_date = l_attendance_block.create_date_;
    if (in_map_id.contains(l_attendance_block.user_ref_id_) == false) {
      continue;
    }
    if (l_get_user_id.contains(in_map_id.at(l_attendance_block.user_ref_id_)) == false) {
      continue;
    }
    l_pre.params.user_ref_id = l_get_user_id.at(in_map_id.at(l_attendance_block.user_ref_id_));
    l_ids.emplace_back(in_comm->operator()(l_pre));
  }

  attendance_block_sub_tab l_sub_tab{};
  auto l_sub_pre = in_comm->prepare(sqlpp::insert_into(l_sub_tab).set(
      l_sub_tab.uuid = sqlpp::parameter(l_sub_tab.uuid), l_sub_tab.start_time = sqlpp::parameter(l_sub_tab.start_time),
      l_sub_tab.end_time = sqlpp::parameter(l_sub_tab.end_time), l_sub_tab.remark = sqlpp::parameter(l_sub_tab.remark),
      l_sub_tab.attendance_type = sqlpp::parameter(l_sub_tab.attendance_type),
      l_sub_tab.dingding_id     = sqlpp::parameter(l_sub_tab.dingding_id),
      l_sub_tab.parent_id       = sqlpp::parameter(l_sub_tab.parent_id)
  ));
  for (std::size_t i = 0; i < in_task.size(); ++i) {
    const auto& l_attendance_block = in_task.at(i);
    const auto& l_id               = l_ids.at(i);
    for (const auto& l_attendance : l_attendance_block.attendance_block_) {
      l_sub_pre.params.uuid            = {l_attendance.id_.begin(), l_attendance.id_.end()};
      l_sub_pre.params.start_time      = l_attendance.start_time_.get_sys_time();
      l_sub_pre.params.end_time        = l_attendance.end_time_.get_sys_time();
      l_sub_pre.params.remark          = l_attendance.remark_;
      l_sub_pre.params.attendance_type = static_cast<std::uint32_t>(l_attendance.type_);
      l_sub_pre.params.dingding_id     = l_attendance.dingding_id_;
      l_sub_pre.params.parent_id       = l_id;
      (*in_comm)(l_sub_pre);
    }
  }
}
void attendance_block::update(const sql_connection_ptr& in_comm, const std::vector<attendance_block>& in_task) {
  attendance_block_tab l_tab{};
  auto l_pre = in_comm->prepare(sqlpp::update(l_tab)
                                   .set(
                                       l_tab.update_time = sqlpp::parameter(l_tab.update_time),
                                       l_tab.create_date = sqlpp::parameter(l_tab.create_date),
                                       l_tab.user_ref_id = sqlpp::parameter(l_tab.user_ref_id)
                                   )
                                   .where(l_tab.uuid == sqlpp::parameter(l_tab.uuid)));
  for (const auto& l_attendance_block : in_task) {
    l_pre.params.update_time = l_attendance_block.update_time_.get_sys_time();
    l_pre.params.create_date = l_attendance_block.create_date_;
    l_pre.params.uuid        = {l_attendance_block.id_.begin(), l_attendance_block.id_.end()};
    (*in_comm)(l_pre);
  }

  // get ids
  std::vector<std::int64_t> l_ids{};
  {
    std::vector<std::vector<std::uint8_t>> l_uuids{};
    std::transform(
        in_task.begin(), in_task.end(), std::back_inserter(l_uuids),
        [](const auto& in_attendance_block) -> std::vector<std::uint8_t> {
          return {in_attendance_block.id_.begin(), in_attendance_block.id_.end()};
        }
    );

    auto l_pre = in_comm->prepare(sqlpp::select(l_tab.id).from(l_tab).where(l_tab.uuid.in(sqlpp::value_list(l_uuids))));
    for (const auto& l_row : (*in_comm)(l_pre)) {
      l_ids.emplace_back(l_row.id);
    }
  }
  // delete all sub
  attendance_block_sub_tab l_sub_tab{};
  (*in_comm)(sqlpp::remove_from(l_sub_tab).where(l_sub_tab.parent_id.in(sqlpp::value_list(l_ids))));
  // insert all sub
  auto l_sub_pre = in_comm->prepare(sqlpp::insert_into(l_sub_tab).set(
      l_sub_tab.uuid = sqlpp::parameter(l_sub_tab.uuid), l_sub_tab.start_time = sqlpp::parameter(l_sub_tab.start_time),
      l_sub_tab.end_time = sqlpp::parameter(l_sub_tab.end_time), l_sub_tab.remark = sqlpp::parameter(l_sub_tab.remark),
      l_sub_tab.attendance_type = sqlpp::parameter(l_sub_tab.attendance_type),
      l_sub_tab.dingding_id     = sqlpp::parameter(l_sub_tab.dingding_id),
      l_sub_tab.parent_id       = sqlpp::parameter(l_sub_tab.parent_id)
  ));
  for (auto i = 0; i < in_task.size(); ++i) {
    const auto& l_attendance_block = in_task.at(i);
    const auto& l_id               = l_ids.at(i);
    for (const auto& l_attendance : l_attendance_block.attendance_block_) {
      l_sub_pre.params.uuid            = {l_attendance.id_.begin(), l_attendance.id_.end()};
      l_sub_pre.params.start_time      = l_attendance.start_time_.get_sys_time();
      l_sub_pre.params.end_time        = l_attendance.end_time_.get_sys_time();
      l_sub_pre.params.remark          = l_attendance.remark_;
      l_sub_pre.params.attendance_type = static_cast<std::uint32_t>(l_attendance.type_);
      l_sub_pre.params.dingding_id     = l_attendance.dingding_id_;
      l_sub_pre.params.parent_id       = l_id;
      (*in_comm)(l_sub_pre);
    }
  }
}
void attendance_block::delete_by_ids(const sql_connection_ptr& in_comm, const std::vector<boost::uuids::uuid>& in_ids) {
  attendance_block_tab l_tab{};
  auto l_pre = in_comm->prepare(sqlpp::remove_from(l_tab).where(l_tab.uuid == sqlpp::parameter(l_tab.uuid)));
  for (const auto& l_id : in_ids) {
    l_pre.params.uuid = {l_id.begin(), l_id.end()};
    (*in_comm)(l_pre);
  }
}

}  // namespace doodle