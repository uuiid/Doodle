//
// Created by TD on 2024/3/13.
//
#include "all.h"
//
#include <doodle_core/core/core_sql.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/sqlite3/sqlite3.h>
#include <doodle_core/sqlite_orm/sqlite_base.h>
//
#include <doodle_core/metadata/server_task_info.h>
//
#include <boost/preprocessor.hpp>

#include <entt/entt.hpp>
#include <sqlpp11/ppgen.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::snapshot {

// clang-format off
SQLPP_DECLARE_TABLE(
    (server_task_info_tab),
    (id,                 int,      SQLPP_PRIMARY_KEY)
    (entity_identifier,  int,      SQLPP_NOT_NULL)
    (data,               text,     SQLPP_NOT_NULL)
    (status,             text,     SQLPP_NOT_NULL)
    (name,               text,     SQLPP_NOT_NULL)
    (source_computer,    text,     SQLPP_NOT_NULL)
    (submitter,          text,     SQLPP_NOT_NULL)
    (submit_time,        datetime, SQLPP_NOT_NULL)
    (run_computer,       text)
    (run_computer_ip,    text)
    (run_time,           datetime)
    (end_time,           datetime)
    (log_path,           text)
)
// clang-format on

namespace {
void create_table(const conn_ptr& in_conn) {
  detail::sql_table_base<server_task_info_tab::server_task_info_tab> l_tab{};
  l_tab.create_table(in_conn);
}
std::shared_ptr<void> begin_save(const conn_ptr& in_conn) {
  server_task_info_tab::server_task_info_tab l_tab{};
  auto l_pre = in_conn->prepare(
      sqlpp::sqlite3::insert_into(l_tab)
          .set(
              l_tab.entity_identifier = sqlpp::parameter(l_tab.entity_identifier),
              l_tab.data = sqlpp::parameter(l_tab.data), l_tab.status = sqlpp::parameter(l_tab.status),
              l_tab.name            = sqlpp::parameter(l_tab.name),
              l_tab.source_computer = sqlpp::parameter(l_tab.source_computer),
              l_tab.submitter       = sqlpp::parameter(l_tab.submitter),
              l_tab.submit_time     = sqlpp::parameter(l_tab.submit_time),
              l_tab.run_computer    = sqlpp::parameter(l_tab.run_computer),
              l_tab.run_computer_ip = sqlpp::parameter(l_tab.run_computer_ip),
              l_tab.run_time = sqlpp::parameter(l_tab.run_time), l_tab.end_time = sqlpp::parameter(l_tab.end_time),
              l_tab.log_path = sqlpp::parameter(l_tab.log_path)
          )
          .on_conflict(l_tab.entity_identifier)
          .do_update(
              l_tab.data = sqlpp::sqlite3::excluded(l_tab.data), l_tab.status = sqlpp::sqlite3::excluded(l_tab.status),
              l_tab.name            = sqlpp::sqlite3::excluded(l_tab.name),
              l_tab.source_computer = sqlpp::sqlite3::excluded(l_tab.source_computer),
              l_tab.submitter       = sqlpp::sqlite3::excluded(l_tab.submitter),
              l_tab.submit_time     = sqlpp::sqlite3::excluded(l_tab.submit_time),
              l_tab.run_computer    = sqlpp::sqlite3::excluded(l_tab.run_computer),
              l_tab.run_computer_ip = sqlpp::sqlite3::excluded(l_tab.run_computer_ip),
              l_tab.run_time        = sqlpp::sqlite3::excluded(l_tab.run_time),
              l_tab.end_time        = sqlpp::sqlite3::excluded(l_tab.end_time),
              l_tab.log_path        = sqlpp::sqlite3::excluded(l_tab.log_path)
          )
  );
  return std::make_shared<decltype(l_pre)>(std::move(l_pre));
}
void save(
    const server_task_info& in_com, entt::entity& in_entity, std::shared_ptr<void>& in_pre, const conn_ptr& in_conn
) {
  server_task_info_tab::server_task_info_tab l_tab{};
  using pre_t                     = decltype(in_conn->prepare(
      sqlpp::sqlite3::insert_into(l_tab)
          .set(
              l_tab.entity_identifier = sqlpp::parameter(l_tab.entity_identifier),
              l_tab.data = sqlpp::parameter(l_tab.data), l_tab.status = sqlpp::parameter(l_tab.status),
              l_tab.name            = sqlpp::parameter(l_tab.name),
              l_tab.source_computer = sqlpp::parameter(l_tab.source_computer),
              l_tab.submitter       = sqlpp::parameter(l_tab.submitter),
              l_tab.submit_time     = sqlpp::parameter(l_tab.submit_time),
              l_tab.run_computer    = sqlpp::parameter(l_tab.run_computer),
              l_tab.run_computer_ip = sqlpp::parameter(l_tab.run_computer_ip),
              l_tab.run_time = sqlpp::parameter(l_tab.run_time), l_tab.end_time = sqlpp::parameter(l_tab.end_time),
              l_tab.log_path = sqlpp::parameter(l_tab.log_path)
          )
          .on_conflict(l_tab.entity_identifier)
          .do_update(
              l_tab.data = sqlpp::sqlite3::excluded(l_tab.data), l_tab.status = sqlpp::sqlite3::excluded(l_tab.status),
              l_tab.name            = sqlpp::sqlite3::excluded(l_tab.name),
              l_tab.source_computer = sqlpp::sqlite3::excluded(l_tab.source_computer),
              l_tab.submitter       = sqlpp::sqlite3::excluded(l_tab.submitter),
              l_tab.submit_time     = sqlpp::sqlite3::excluded(l_tab.submit_time),
              l_tab.run_computer    = sqlpp::sqlite3::excluded(l_tab.run_computer),
              l_tab.run_computer_ip = sqlpp::sqlite3::excluded(l_tab.run_computer_ip),
              l_tab.run_time        = sqlpp::sqlite3::excluded(l_tab.run_time),
              l_tab.end_time        = sqlpp::sqlite3::excluded(l_tab.end_time),
              l_tab.log_path        = sqlpp::sqlite3::excluded(l_tab.log_path)
          )
  ));
  auto l_pre                      = std::static_pointer_cast<pre_t>(in_pre);
  l_pre->params.entity_identifier = enum_to_num(in_entity);
  l_pre->params.data              = in_com.data_.dump();
  l_pre->params.status            = magic_enum::enum_name(in_com.status_);
  l_pre->params.name              = in_com.name_;
  l_pre->params.source_computer   = in_com.source_computer_;
  l_pre->params.submitter         = in_com.submitter_;
  l_pre->params.submit_time =
      chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(in_com.submit_time_);
  l_pre->params.run_computer    = in_com.run_computer_;
  l_pre->params.run_computer_ip = in_com.run_computer_ip_;
  l_pre->params.run_time = chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(in_com.run_time_);
  l_pre->params.end_time = chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(in_com.end_time_);
  l_pre->params.log_path = in_com.log_path_.generic_string();
  (*in_conn)(*l_pre);
}
void destroy(const std::vector<std::int64_t>& in_vector, const conn_ptr& in_conn) {
  detail::sql_com_destroy<server_task_info_tab::server_task_info_tab>(in_conn, in_vector);
}
std::underlying_type_t<entt::entity> get_size(const conn_ptr& in_conn) {
  server_task_info_tab::server_task_info_tab l_tabl{};
  for (auto&& raw : (*in_conn)(sqlpp::select(sqlpp::count(l_tabl.id)).from(l_tabl).unconditionally())) {
    return raw.count.value();
  }
  return 0;
}
using pre_rus_t = decltype(std::declval<sqlpp::sqlite3::connection>()(
    sqlpp::select(
        std::declval<server_task_info_tab::server_task_info_tab>().entity_identifier,
        std::declval<server_task_info_tab::server_task_info_tab>().data,
        std::declval<server_task_info_tab::server_task_info_tab>().status,
        std::declval<server_task_info_tab::server_task_info_tab>().name,
        std::declval<server_task_info_tab::server_task_info_tab>().source_computer,
        std::declval<server_task_info_tab::server_task_info_tab>().submitter,
        std::declval<server_task_info_tab::server_task_info_tab>().submit_time,
        std::declval<server_task_info_tab::server_task_info_tab>().run_computer,
        std::declval<server_task_info_tab::server_task_info_tab>().run_computer_ip,
        std::declval<server_task_info_tab::server_task_info_tab>().run_time,
        std::declval<server_task_info_tab::server_task_info_tab>().end_time,
        std::declval<server_task_info_tab::server_task_info_tab>().log_path
    )
        .from(std::declval<server_task_info_tab::server_task_info_tab>())
        .unconditionally()
));
std::shared_ptr<void> begin_load(const conn_ptr& in_conn) {
  server_task_info_tab::server_task_info_tab l_tabl{};
  auto l_r = (*in_conn)(sqlpp::select(
                            l_tabl.entity_identifier, l_tabl.data, l_tabl.status, l_tabl.name, l_tabl.source_computer,
                            l_tabl.submitter, l_tabl.submit_time, l_tabl.run_computer, l_tabl.run_computer_ip,
                            l_tabl.run_time, l_tabl.end_time, l_tabl.log_path
  )
                            .from(l_tabl)
                            .unconditionally());
  return std::make_shared<decltype(l_r)>(std::move(l_r));
}
void load_entt(entt::entity& in_entity, std::shared_ptr<void>& in_pre) {
  auto l_r  = std::static_pointer_cast<pre_rus_t>(in_pre);
  in_entity = num_to_enum<entt::entity>(l_r->front().entity_identifier.value());
}
void load_com(server_task_info& in_entity, std::shared_ptr<void>& in_pre) {
  auto l_r          = std::static_pointer_cast<pre_rus_t>(in_pre);
  in_entity.data_   = nlohmann::json::parse(l_r->front().data.value());
  in_entity.status_ = magic_enum::enum_cast<server_task_info_status>(l_r->front().status.value())
                          .value_or(server_task_info_status::unknown);
  in_entity.name_            = l_r->front().name.value();
  in_entity.source_computer_ = l_r->front().source_computer.value();
  in_entity.submitter_       = l_r->front().submitter.value();
  in_entity.submit_time_  = chrono::time_point_cast<chrono::system_clock::duration>(l_r->front().submit_time.value());
  in_entity.run_computer_ = l_r->front().run_computer.value();
  in_entity.run_computer_ip_ = l_r->front().run_computer_ip.value();
  in_entity.run_time_        = chrono::time_point_cast<chrono::system_clock::duration>(l_r->front().run_time.value());
  in_entity.end_time_        = chrono::time_point_cast<chrono::system_clock::duration>(l_r->front().end_time.value());
  in_entity.log_path_        = std::filesystem::path(l_r->front().log_path.value());
}
bool has_table(const conn_ptr& in_conn) {
  detail::sql_table_base<server_task_info_tab::server_task_info_tab> l_tab{};
  return l_tab.has_table(in_conn);
}
}  // namespace
}  // namespace doodle::snapshot