//
// Created by TD on 2024/2/27.
//

#include "server_task_info.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sql.h>
#include <doodle_core/database_task/details/column.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/sqlite3/sqlite3.h>

#include <boost/preprocessor.hpp>

#include <entt/entt.hpp>
#include <sqlpp11/ppgen.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle {

namespace {
DOODLE_SQL_COLUMN_IMP(uuid, sqlpp::blob, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(exe, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(status, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(name, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(source_computer, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(submitter, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(submit_time, sqlpp::time_point, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(run_computer, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(run_computer_ip, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(run_time, sqlpp::time_point, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(end_time, sqlpp::time_point, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(log_path, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(ref_id, sqlpp::blob, database_n::detail::can_be_null);

DOODLE_SQL_TABLE_IMP(
    server_task_info_tab, tables::column::id, uuid, exe, status, name, source_computer, submitter, submit_time,
    run_computer, run_computer_ip, run_time, end_time, log_path, ref_id
);

DOODLE_SQL_COLUMN_IMP(command_task_id, sqlpp::integer, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(command_index, sqlpp::integer, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(command_value, sqlpp::text, database_n::detail::can_be_null);

DOODLE_SQL_TABLE_IMP(command_tab, tables::column::id, command_task_id, command_index, command_value);
}  // namespace

std::vector<server_task_info> server_task_info::select_all(pooled_connection& in_comm) {
  server_task_info_tab l_tab{};
  std::vector<server_task_info> l_res{};
  std::vector<std::int64_t> l_ids{};
  for (auto&& l_row : in_comm(sqlpp::select(sqlpp::all_of(l_tab)).from(l_tab).unconditionally())) {
    server_task_info l_info{};
    std::copy_n(l_row.uuid.value().begin(), l_info.id_.size(), l_info.id_.begin());
    l_info.exe_ = l_row.exe.value();
    l_info.status_ =
        magic_enum::enum_cast<server_task_info_status>(l_row.status.value()).value_or(server_task_info_status::unknown);
    l_info.name_            = l_row.name.value();
    l_info.source_computer_ = l_row.source_computer.value();
    l_info.submitter_       = l_row.submitter.value();
    l_info.submit_time_     = chrono::time_point_cast<chrono::sys_time_pos::clock::duration>(l_row.submit_time.value());
    l_info.run_computer_    = l_row.run_computer.value();
    l_info.run_computer_ip_ = l_row.run_computer_ip.value();
    l_info.run_time_        = chrono::time_point_cast<chrono::sys_time_pos::clock::duration>(l_row.run_time.value());
    l_info.end_time_        = chrono::time_point_cast<chrono::sys_time_pos::clock::duration>(l_row.end_time.value());
    l_info.log_path_        = l_row.log_path.value();
    std::copy_n(l_row.ref_id.value().begin(), l_info.ref_id_.size(), l_info.ref_id_.begin());
    l_res.emplace_back(std::move(l_info));
    l_ids.emplace_back(l_row.id.value());
  }

  command_tab l_command_tab{};
  auto l_pre_com =
      in_comm.prepare(sqlpp::select(sqlpp::all_of(l_command_tab))
                          .from(l_command_tab)
                          .order_by(l_command_tab.command_index.asc())
                          .where(l_command_tab.command_task_id == sqlpp::parameter(l_command_tab.command_task_id)));
  for (auto&& l_id : l_ids) {
    std::vector<std::string> l_command{};
    l_pre_com.params.command_task_id = l_id;
    for (auto&& l_row : in_comm(l_pre_com)) {
      l_command.emplace_back(l_row.command_value.value());
    }
    l_res[std::distance(l_ids.begin(), std::find(l_ids.begin(), l_ids.end(), l_id))].command_ = std::move(l_command);
  }

  return l_res;
}

std::vector<bool> server_task_info::filter_exist(
    pooled_connection& in_comm, const std::vector<server_task_info>& in_task
) {
  server_task_info_tab l_tab{};
  std::vector<bool> l_res{};
  auto l_pre = in_comm.prepare(
      sqlpp::select(sqlpp::count(l_tab.id)).from(l_tab).where(l_tab.uuid == sqlpp::parameter(l_tab.uuid))
  );
  for (auto i = 0; i < in_task.size(); ++i) {
    l_pre.params.uuid = {in_task[i].id_.begin(), in_task[i].id_.end()};
    for (auto&& l_row : in_comm(l_pre)) {
      l_res.emplace_back(l_row.count.value() > 0);
    }
  }
  return l_res;
}

void server_task_info::insert(pooled_connection& in_comm, const std::vector<server_task_info>& in_task) {
  server_task_info_tab l_tab{};
  auto l_pre = in_comm.prepare(
      sqlpp::sqlite3::insert_into(l_tab)
          .set(
              l_tab.exe = sqlpp::parameter(l_tab.exe), l_tab.status = sqlpp::parameter(l_tab.status),
              l_tab.uuid = sqlpp::parameter(l_tab.uuid), l_tab.name = sqlpp::parameter(l_tab.name),
              l_tab.source_computer = sqlpp::parameter(l_tab.source_computer),
              l_tab.submitter       = sqlpp::parameter(l_tab.submitter),
              l_tab.submit_time     = sqlpp::parameter(l_tab.submit_time),
              l_tab.run_computer    = sqlpp::parameter(l_tab.run_computer),
              l_tab.run_computer_ip = sqlpp::parameter(l_tab.run_computer_ip),
              l_tab.run_time = sqlpp::parameter(l_tab.run_time), l_tab.end_time = sqlpp::parameter(l_tab.end_time),
              l_tab.log_path = sqlpp::parameter(l_tab.log_path), l_tab.ref_id = sqlpp::parameter(l_tab.ref_id)
          )
          .on_conflict(l_tab.uuid)
          .do_update(
              l_tab.exe = sqlpp::sqlite3::excluded(l_tab.exe), l_tab.status = sqlpp::sqlite3::excluded(l_tab.status),
              l_tab.name            = sqlpp::sqlite3::excluded(l_tab.name),
              l_tab.source_computer = sqlpp::sqlite3::excluded(l_tab.source_computer),
              l_tab.submitter       = sqlpp::sqlite3::excluded(l_tab.submitter),
              l_tab.submit_time     = sqlpp::sqlite3::excluded(l_tab.submit_time),
              l_tab.run_computer    = sqlpp::sqlite3::excluded(l_tab.run_computer),
              l_tab.run_computer_ip = sqlpp::sqlite3::excluded(l_tab.run_computer_ip),
              l_tab.run_time        = sqlpp::sqlite3::excluded(l_tab.run_time),
              l_tab.end_time        = sqlpp::sqlite3::excluded(l_tab.end_time),
              l_tab.log_path        = sqlpp::sqlite3::excluded(l_tab.log_path),
              l_tab.ref_id          = sqlpp::sqlite3::excluded(l_tab.ref_id)
          )
  );
  std::vector<std::int64_t> l_ids{};
  for (auto&& l_task : in_task) {
    l_pre.params.uuid            = {l_task.id_.begin(), l_task.id_.end()};
    l_pre.params.exe             = l_task.exe_;
    l_pre.params.status          = magic_enum::enum_name(l_task.status_);
    l_pre.params.name            = l_task.name_;
    l_pre.params.source_computer = l_task.source_computer_;
    l_pre.params.submitter       = l_task.submitter_;
    l_pre.params.submit_time =
        chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_task.submit_time_);
    l_pre.params.run_computer    = l_task.run_computer_;
    l_pre.params.run_computer_ip = l_task.run_computer_ip_;
    l_pre.params.run_time = chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_task.run_time_);
    l_pre.params.end_time = chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_task.end_time_);
    l_pre.params.log_path = l_task.log_path_.generic_string();
    l_pre.params.ref_id   = {l_task.ref_id_.begin(), l_task.ref_id_.end()};
    l_ids.emplace_back(in_comm(l_pre));
  }

  command_tab l_command_tab{};
  auto l_pre_com =
      in_comm.prepare(sqlpp::sqlite3::insert_into(l_command_tab)
                          .set(
                              l_command_tab.command_task_id = sqlpp::parameter(l_command_tab.command_task_id),
                              l_command_tab.command_index   = sqlpp::parameter(l_command_tab.command_index),
                              l_command_tab.command_value   = sqlpp::parameter(l_command_tab.command_value)
                          ));
  for (auto i = 0; i < in_task.size(); ++i) {
    for (auto j = 0; j < in_task[i].command_.size(); ++j) {
      l_pre_com.params.command_task_id = l_ids[i];
      l_pre_com.params.command_index   = j;
      l_pre_com.params.command_value   = in_task[i].command_[j];
      in_comm(l_pre_com);
    }
  }
}

void server_task_info::update(pooled_connection& in_comm, const std::vector<server_task_info>& in_task) {
  server_task_info_tab l_tab{};
  auto l_pre = in_comm.prepare(
      sqlpp::update(l_tab)
          .set(
              l_tab.exe = sqlpp::parameter(l_tab.exe), l_tab.status = sqlpp::parameter(l_tab.status),
              l_tab.name            = sqlpp::parameter(l_tab.name),
              l_tab.source_computer = sqlpp::parameter(l_tab.source_computer),
              l_tab.submitter       = sqlpp::parameter(l_tab.submitter),
              l_tab.submit_time     = sqlpp::parameter(l_tab.submit_time),
              l_tab.run_computer    = sqlpp::parameter(l_tab.run_computer),
              l_tab.run_computer_ip = sqlpp::parameter(l_tab.run_computer_ip),
              l_tab.run_time = sqlpp::parameter(l_tab.run_time), l_tab.end_time = sqlpp::parameter(l_tab.end_time),
              l_tab.log_path = sqlpp::parameter(l_tab.log_path), l_tab.ref_id = sqlpp::parameter(l_tab.ref_id)
          )
          .where(l_tab.uuid == sqlpp::parameter(l_tab.uuid))
  );
  for (auto&& l_task : in_task) {
    l_pre.params.uuid            = {l_task.id_.begin(), l_task.id_.end()};
    l_pre.params.exe             = l_task.exe_;
    l_pre.params.status          = magic_enum::enum_name(l_task.status_);
    l_pre.params.name            = l_task.name_;
    l_pre.params.source_computer = l_task.source_computer_;
    l_pre.params.submitter       = l_task.submitter_;
    l_pre.params.submit_time =
        chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_task.submit_time_);
    l_pre.params.run_computer    = l_task.run_computer_;
    l_pre.params.run_computer_ip = l_task.run_computer_ip_;
    l_pre.params.run_time = chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_task.run_time_);
    l_pre.params.end_time = chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_task.end_time_);
    l_pre.params.log_path = l_task.log_path_.generic_string();
    l_pre.params.ref_id   = {l_task.ref_id_.begin(), l_task.ref_id_.end()};
    in_comm(l_pre);
  }
}

void server_task_info::delete_by_ids(pooled_connection& in_comm, const std::vector<boost::uuids::uuid>& in_ids) {
  server_task_info_tab l_tab{};
  auto l_pre = in_comm.prepare(sqlpp::remove_from(l_tab).where(l_tab.uuid == sqlpp::parameter(l_tab.uuid)));
  for (auto&& l_id : in_ids) {
    l_pre.params.uuid = {l_id.begin(), l_id.end()};
    in_comm(l_pre);
  }
}

void server_task_info::create_table(pooled_connection& in_comm) {
  in_comm.execute(R"(
CREATE TABLE IF NOT EXISTS server_task_info_tab (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    uuid            BLOB NOT NULL UNIQUE,
    exe             TEXT ,
    status          TEXT ,
    name            TEXT ,
    source_computer TEXT ,
    submitter       TEXT ,
    submit_time     TIMESTAMP ,
    run_computer    TEXT ,
    run_computer_ip TEXT ,
    run_time        TIMESTAMP ,
    end_time        TIMESTAMP ,
    log_path        TEXT ,
    ref_id          BLOB
    )
  )");
  in_comm.execute(R"(
CREATE TABLE IF NOT EXISTS command_tab (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    command_task_id  INTEGER ,
    command_index    INTEGER ,
    command_value    TEXT ,
    FOREIGN KEY (command_task_id) REFERENCES server_task_info_tab(id) ON DELETE CASCADE ON UPDATE CASCADE
    )
  )");
}

std::string server_task_info::read_log(level::level_enum in_level) const {
  auto l_path = get_log_path(in_level);
  if (!FSys::exists(l_path)) return "";
  FSys::ifstream l_file{l_path};
  return std::string{std::istream_iterator<char>{l_file}, std::istream_iterator<char>{}};
}
FSys::path server_task_info::get_log_path(level::level_enum in_level) const {
  return core_set::get_set().get_cache_root(log_path_) / fmt::format("{}.log.txt", magic_enum::enum_name(in_level));
}
void server_task_info::write_log(level::level_enum in_level, std::string_view in_msg) {
  FSys::ofstream{get_log_path(in_level), std::ios::app | std::ios::binary} << in_msg;
}
bool server_task_info::operator==(const doodle::server_task_info& in_rhs) const {
  return std::tie(
             exe_, command_, status_, name_, source_computer_, submitter_, submit_time_, run_computer_,
             run_computer_ip_, run_time_, end_time_, log_path_, ref_id_
         ) ==
         std::tie(
             in_rhs.exe_, command_, in_rhs.status_, in_rhs.name_, in_rhs.source_computer_, in_rhs.submitter_,
             in_rhs.submit_time_, in_rhs.run_computer_, in_rhs.run_computer_ip_, in_rhs.run_time_, in_rhs.end_time_,
             in_rhs.log_path_, in_rhs.ref_id_
         );
}
}  // namespace doodle