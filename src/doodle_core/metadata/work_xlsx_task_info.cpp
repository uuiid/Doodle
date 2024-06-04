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
DOODLE_SQL_COLUMN_IMP(remark, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(user_remark, sqlpp::text, database_n::detail::can_be_null);

DOODLE_SQL_COLUMN_IMP(user_ref_id, sqlpp::integer, database_n::detail::can_be_null);

DOODLE_SQL_COLUMN_IMP(parent_id, sqlpp::integer, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(index_col, sqlpp::integer, database_n::detail::can_be_null);

DOODLE_SQL_TABLE_IMP(work_xlsx_task_info_block_tab, tables::column::id, uuid, year_c, month_c, duration, user_ref_id);
DOODLE_SQL_TABLE_IMP(
    work_xlsx_task_info_block_sub_tab, tables::column::id, uuid, start_time, end_time, duration, kitsu_task_ref_id,
    parent_id, index_col, remark, user_remark
);

}  // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<work_xlsx_task_info_block> work_xlsx_task_info_block::select_all(
    pooled_connection& in_comm, const std::map<boost::uuids::uuid, entt::entity>& in_map_id
) {
  auto l_user_id_maps = user::select_all_map_id(in_comm);
  work_xlsx_task_info_block_tab l_tab{};

  std::vector<work_xlsx_task_info_block> l_ret{};
  std::vector<std::int64_t> l_ret_id{};
  std::map<std::int64_t, std::size_t> l_map_id{};
  for (auto&& l_row : in_comm(sqlpp::select(sqlpp::all_of(l_tab)).from(l_tab).unconditionally())) {
    work_xlsx_task_info_block l_info{};
    std::copy_n(l_row.uuid.value().begin(), l_info.id_.size(), l_info.id_.begin());
    l_info.year_month_ = chrono::year_month{
        chrono::year{boost::numeric_cast<std::int32_t>(l_row.year_c.value())},
        chrono::month{boost::numeric_cast<std::uint32_t>(l_row.month_c.value())}
    };
    l_info.duration_ = chrono::system_clock::duration{l_row.duration.value()};
    if (!l_user_id_maps.contains(l_row.user_ref_id.value())) continue;
    l_info.user_refs_ = in_map_id.at(l_user_id_maps.at(l_row.user_ref_id.value()));
    l_ret.emplace_back(std::move(l_info));
    l_map_id.emplace(l_row.id.value(), l_ret.size() - 1);
  }

  work_xlsx_task_info_block_sub_tab l_sub_tab{};
  auto l_pre = in_comm.prepare(sqlpp::select(sqlpp::all_of(l_sub_tab))
                                   .from(l_sub_tab)
                                   .order_by(l_sub_tab.index_col.asc())
                                   .where(l_sub_tab.parent_id == sqlpp::parameter(l_sub_tab.parent_id)));
  for (auto&& [l_id, l_index] : l_map_id) {
    l_pre.params.parent_id = l_id;
    for (auto&& l_row : in_comm(l_pre)) {
      work_xlsx_task_info l_info{};
      std::copy_n(l_row.uuid.value().begin(), l_info.id_.size(), l_info.id_.begin());
      l_info.start_time_  = l_row.start_time.value();
      l_info.end_time_    = l_row.end_time.value();
      l_info.duration_    = chrono::system_clock::duration{l_row.duration.value()};
      l_info.remark_      = l_row.remark.value();
      l_info.user_remark_ = l_row.user_remark.value();
      if (!l_row.kitsu_task_ref_id.is_null())
        std::copy_n(
            l_row.kitsu_task_ref_id.value().begin(), l_info.kitsu_task_ref_id_.size(), l_info.kitsu_task_ref_id_.begin()
        );
      l_ret[l_index].task_info_.emplace_back(std::move(l_info));
    }
  }

  return l_ret;
}
void work_xlsx_task_info_block::create_table(pooled_connection& in_comm) {
  in_comm.execute(R"(
    CREATE TABLE IF NOT EXISTS work_xlsx_task_info_block_tab (
      id          INTEGER    PRIMARY KEY NOT NULL,
      uuid        BLOB       NOT NULL UNIQUE,
      year_c      INTEGER,
      month_c     INTEGER,
      duration    INTEGER,
      user_ref_id INTEGER     REFERENCES user_tab(id) ON DELETE CASCADE
    );
  )");
  in_comm.execute(R"(
    CREATE INDEX IF NOT EXISTS work_xlsx_task_info_block_tab_uuid_index ON work_xlsx_task_info_block_tab (uuid);
  )");
  in_comm.execute(R"(
    CREATE TABLE IF NOT EXISTS work_xlsx_task_info_block_sub_tab (
      id                INTEGER    PRIMARY KEY NOT NULL,
      uuid              BLOB       NOT NULL UNIQUE,
      start_time        TIMESTAMP,
      end_time          TIMESTAMP,
      duration          INTEGER,
      kitsu_task_ref_id BLOB,
      parent_id         INTEGER     REFERENCES work_xlsx_task_info_block_tab(id) ON DELETE CASCADE,
      index_col         INTEGER,
      remark            TEXT,
      user_remark       TEXT
    );
  )");
  in_comm.execute(R"(
    CREATE INDEX IF NOT EXISTS work_xlsx_task_info_block_sub_tab_uuid_index ON work_xlsx_task_info_block_sub_tab (uuid);
  )");
}

std::vector<bool> work_xlsx_task_info_block::filter_exist(
    pooled_connection& in_comm, const std::vector<work_xlsx_task_info_block>& in_task
) {
  work_xlsx_task_info_block_tab l_tab{};
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
void work_xlsx_task_info_block::insert(
    pooled_connection& in_comm, const std::vector<work_xlsx_task_info_block>& in_task,
    const std::map<entt::entity, boost::uuids::uuid>& in_map_id
) {
  auto l_user_id_maps = user::select_all_map_id(in_comm);
  // 反转key value
  std::map<boost::uuids::uuid, std::int64_t> l_user_uuid_maps{};
  for (const auto& [l_id, l_uuid] : l_user_id_maps) {
    l_user_uuid_maps[l_uuid] = l_id;
  }

  work_xlsx_task_info_block_tab l_tab{};
  auto l_pre = in_comm.prepare(sqlpp::insert_into(l_tab).set(
      l_tab.uuid = sqlpp::parameter(l_tab.uuid), l_tab.year_c = sqlpp::parameter(l_tab.year_c),
      l_tab.month_c = sqlpp::parameter(l_tab.month_c), l_tab.duration = sqlpp::parameter(l_tab.duration),
      l_tab.user_ref_id = sqlpp::parameter(l_tab.user_ref_id)
  ));
  std::vector<std::int64_t> l_ids{};
  for (const auto& l_info : in_task) {
    l_pre.params.uuid        = {l_info.id_.begin(), l_info.id_.end()};
    l_pre.params.year_c      = std::int32_t(l_info.year_month_.year());
    l_pre.params.month_c     = std::uint32_t(l_info.year_month_.month());
    l_pre.params.duration    = l_info.duration_.count();
    l_pre.params.user_ref_id = l_user_uuid_maps.at(in_map_id.at(l_info.user_refs_));
    l_ids.emplace_back(in_comm(l_pre));
  }
  work_xlsx_task_info_block_sub_tab l_sub_tab{};
  auto l_pre_sub = in_comm.prepare(sqlpp::insert_into(l_sub_tab).set(
      l_sub_tab.uuid = sqlpp::parameter(l_sub_tab.uuid), l_sub_tab.start_time = sqlpp::parameter(l_sub_tab.start_time),
      l_sub_tab.end_time          = sqlpp::parameter(l_sub_tab.end_time),
      l_sub_tab.duration          = sqlpp::parameter(l_sub_tab.duration),
      l_sub_tab.kitsu_task_ref_id = sqlpp::parameter(l_sub_tab.kitsu_task_ref_id),
      l_sub_tab.parent_id         = sqlpp::parameter(l_sub_tab.parent_id),
      l_sub_tab.index_col         = sqlpp::parameter(l_sub_tab.index_col),
      l_sub_tab.remark            = sqlpp::parameter(l_sub_tab.remark),
      l_sub_tab.user_remark       = sqlpp::parameter(l_sub_tab.user_remark)
  ));
  for (std::size_t i = 0; i < in_task.size(); ++i) {
    for (std::size_t j = 0; j < in_task[i].task_info_.size(); ++j) {
      l_pre_sub.params.uuid       = {in_task[i].task_info_[j].id_.begin(), in_task[i].task_info_[j].id_.end()};
      l_pre_sub.params.start_time = chrono::time_point_cast<chrono::microseconds>(in_task[i].task_info_[j].start_time_);
      l_pre_sub.params.end_time   = chrono::time_point_cast<chrono::microseconds>(in_task[i].task_info_[j].end_time_);

      l_pre_sub.params.duration   = in_task[i].task_info_[j].duration_.count();
      l_pre_sub.params.kitsu_task_ref_id = {
          in_task[i].task_info_[j].kitsu_task_ref_id_.begin(), in_task[i].task_info_[j].kitsu_task_ref_id_.end()
      };
      l_pre_sub.params.parent_id   = l_ids[i];
      l_pre_sub.params.index_col   = j;
      l_pre_sub.params.remark      = in_task[i].task_info_[j].remark_;
      l_pre_sub.params.user_remark = in_task[i].task_info_[j].user_remark_;

      in_comm(l_pre_sub);
    }
  }
}
void work_xlsx_task_info_block::update(
    pooled_connection& in_comm, const std::vector<work_xlsx_task_info_block>& in_task
) {
  work_xlsx_task_info_block_tab l_tab{};
  auto l_pre = in_comm.prepare(sqlpp::update(l_tab)
                                   .set(
                                       l_tab.year_c   = sqlpp::parameter(l_tab.year_c),
                                       l_tab.month_c  = sqlpp::parameter(l_tab.month_c),
                                       l_tab.duration = sqlpp::parameter(l_tab.duration)
                                   )
                                   .where(l_tab.uuid == sqlpp::parameter(l_tab.uuid)));
  for (const auto& l_info : in_task) {
    l_pre.params.year_c   = std::int32_t(l_info.year_month_.year());
    l_pre.params.month_c  = std::uint32_t(l_info.year_month_.month());
    l_pre.params.duration = l_info.duration_.count();
    l_pre.params.uuid     = {l_info.id_.begin(), l_info.id_.end()};
    in_comm(l_pre);
  }
  // select id
  std::vector<std::int64_t> l_ids{};
  {
    auto l_pre2 =
        in_comm.prepare(sqlpp::select(l_tab.id).from(l_tab).where(l_tab.uuid == sqlpp::parameter(l_tab.uuid)));

    for (auto&& l_info : in_task) {
      l_pre2.params.uuid = {l_info.id_.begin(), l_info.id_.end()};
      for (auto&& l_row : in_comm(l_pre2)) {
        l_ids.emplace_back(l_row.id.value());
      }
    }
  }

  work_xlsx_task_info_block_sub_tab l_sub_tab{};
  auto l_pre_sub =
      in_comm.prepare(sqlpp::sqlite3::insert_into(l_sub_tab)
                          .set(
                              l_sub_tab.uuid              = sqlpp::parameter(l_sub_tab.uuid),
                              l_sub_tab.start_time        = sqlpp::parameter(l_sub_tab.start_time),
                              l_sub_tab.end_time          = sqlpp::parameter(l_sub_tab.end_time),
                              l_sub_tab.duration          = sqlpp::parameter(l_sub_tab.duration),
                              l_sub_tab.kitsu_task_ref_id = sqlpp::parameter(l_sub_tab.kitsu_task_ref_id),
                              l_sub_tab.parent_id         = sqlpp::parameter(l_sub_tab.parent_id),
                              l_sub_tab.index_col         = sqlpp::parameter(l_sub_tab.index_col),
                              l_sub_tab.remark            = sqlpp::parameter(l_sub_tab.remark),
                              l_sub_tab.user_remark       = sqlpp::parameter(l_sub_tab.user_remark)
                          )
                          .on_conflict(l_sub_tab.uuid)
                          .do_update(
                              l_sub_tab.start_time        = sqlpp::sqlite3::excluded(l_sub_tab.start_time),
                              l_sub_tab.end_time          = sqlpp::sqlite3::excluded(l_sub_tab.end_time),
                              l_sub_tab.duration          = sqlpp::sqlite3::excluded(l_sub_tab.duration),
                              l_sub_tab.kitsu_task_ref_id = sqlpp::sqlite3::excluded(l_sub_tab.kitsu_task_ref_id),
                              l_sub_tab.parent_id         = sqlpp::sqlite3::excluded(l_sub_tab.parent_id),
                              l_sub_tab.index_col         = sqlpp::sqlite3::excluded(l_sub_tab.index_col),
                              l_sub_tab.remark            = sqlpp::sqlite3::excluded(l_sub_tab.remark),
                              l_sub_tab.user_remark       = sqlpp::sqlite3::excluded(l_sub_tab.user_remark)
                          ));
  for (std::size_t i = 0; i < in_task.size(); ++i) {
    for (std::size_t j = 0; j < in_task[i].task_info_.size(); ++j) {
      l_pre_sub.params.uuid       = {in_task[i].task_info_[j].id_.begin(), in_task[i].task_info_[j].id_.end()};
      l_pre_sub.params.start_time = chrono::time_point_cast<chrono::microseconds>(in_task[i].task_info_[j].start_time_);
      l_pre_sub.params.end_time   = chrono::time_point_cast<chrono::microseconds>(in_task[i].task_info_[j].end_time_);
      l_pre_sub.params.duration   = in_task[i].task_info_[j].duration_.count();
      l_pre_sub.params.kitsu_task_ref_id = {
          in_task[i].task_info_[j].kitsu_task_ref_id_.begin(), in_task[i].task_info_[j].kitsu_task_ref_id_.end()
      };
      l_pre_sub.params.parent_id   = l_ids[i];
      l_pre_sub.params.index_col   = j;
      l_pre_sub.params.remark      = in_task[i].task_info_[j].remark_;
      l_pre_sub.params.user_remark = in_task[i].task_info_[j].user_remark_;
      in_comm(l_pre_sub);
    }
  }
}
void work_xlsx_task_info_block::delete_by_ids(
    pooled_connection& in_comm, const std::vector<boost::uuids::uuid>& in_ids
) {
  work_xlsx_task_info_block_tab l_tab{};
  auto l_pre = in_comm.prepare(sqlpp::remove_from(l_tab).where(l_tab.uuid == sqlpp::parameter(l_tab.uuid)));
  for (const auto& l_id : in_ids) {
    l_pre.params.uuid = {l_id.begin(), l_id.end()};
    in_comm(l_pre);
  }
}

// to json
void to_json(nlohmann::json& j, const work_xlsx_task_info& p) {
  j["id"]                = p.id_;
  j["start_time"]        = fmt::format("{:%FT%T}", p.start_time_);
  j["end_time"]          = fmt::format("{:%FT%T}", p.end_time_);
  j["duration"]          = p.duration_.count();
  j["remark"]            = p.remark_;
  j["user_remark"]       = p.user_remark_;
  j["kitsu_task_ref_id"] = p.kitsu_task_ref_id_;
}

}  // namespace doodle