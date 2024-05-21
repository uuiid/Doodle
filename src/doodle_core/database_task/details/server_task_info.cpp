//
// Created by TD on 2024/3/7.
//

#include "server_task_info.h"
namespace doodle::database_n {
void sql_com<doodle::server_task_info>::insert(doodle::conn_ptr &in_ptr, const std::vector<entt::handle> &in_id) {
  auto &l_conn = *in_ptr;

  tables::server_task_info l_tabl{};
  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_tabl).set(
      l_tabl.entity_id = sqlpp::parameter(l_tabl.entity_id), l_tabl.data = sqlpp::parameter(l_tabl.data),
      l_tabl.status = sqlpp::parameter(l_tabl.status), l_tabl.name = sqlpp::parameter(l_tabl.name),
      l_tabl.source_computer = sqlpp::parameter(l_tabl.source_computer),
      l_tabl.submitter = sqlpp::parameter(l_tabl.submitter), l_tabl.submit_time = sqlpp::parameter(l_tabl.submit_time),
      l_tabl.run_computer    = sqlpp::parameter(l_tabl.run_computer),
      l_tabl.run_computer_ip = sqlpp::parameter(l_tabl.run_computer_ip),
      l_tabl.run_time = sqlpp::parameter(l_tabl.run_time), l_tabl.end_time = sqlpp::parameter(l_tabl.end_time),
      l_tabl.log_path = sqlpp::parameter(l_tabl.log_path)
  ));

  for (auto &l_h : in_id) {
    auto &l_server_task_info     = l_h.get<server_task_info>();
    l_pre.params.entity_id       = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_pre.params.status          = enum_to_num(l_server_task_info.status_);
    l_pre.params.name            = l_server_task_info.name_;
    l_pre.params.source_computer = l_server_task_info.source_computer_;
    l_pre.params.submitter       = l_server_task_info.submitter_;
    l_pre.params.submit_time =
        chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_server_task_info.submit_time_);
    l_pre.params.run_computer    = l_server_task_info.run_computer_;
    l_pre.params.run_computer_ip = l_server_task_info.run_computer_ip_;
    l_pre.params.run_time =
        chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_server_task_info.run_time_);
    l_pre.params.end_time =
        chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_server_task_info.end_time_);
    l_pre.params.log_path = l_server_task_info.log_path_.generic_string();
    l_conn(l_pre);
  }
}
void sql_com<doodle::server_task_info>::update(
    doodle::conn_ptr &in_ptr, const std::map<std::int64_t, entt::handle> &in_id
) {
  auto &l_conn = *in_ptr;

  tables::server_task_info l_table{};

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_table)
          .set(
              l_table.data = sqlpp::parameter(l_table.data), l_table.status = sqlpp::parameter(l_table.status),
              l_table.name            = sqlpp::parameter(l_table.name),
              l_table.source_computer = sqlpp::parameter(l_table.source_computer),
              l_table.submitter       = sqlpp::parameter(l_table.submitter),
              l_table.submit_time     = sqlpp::parameter(l_table.submit_time),
              l_table.run_computer    = sqlpp::parameter(l_table.run_computer),
              l_table.run_computer_ip = sqlpp::parameter(l_table.run_computer_ip),
              l_table.run_time        = sqlpp::parameter(l_table.run_time),
              l_table.end_time        = sqlpp::parameter(l_table.end_time),
              l_table.log_path        = sqlpp::parameter(l_table.log_path)
          )
          .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id) && l_table.id == sqlpp::parameter(l_table.id))
  );

  for (auto &[id, l_h] : in_id) {
    auto &l_server_task_info     = l_h.get<server_task_info>();
    l_pre.params.status          = enum_to_num(l_server_task_info.status_);
    l_pre.params.name            = l_server_task_info.name_;
    l_pre.params.source_computer = l_server_task_info.source_computer_;
    l_pre.params.submitter       = l_server_task_info.submitter_;
    l_pre.params.submit_time =
        chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_server_task_info.submit_time_);
    l_pre.params.run_computer    = l_server_task_info.run_computer_;
    l_pre.params.run_computer_ip = l_server_task_info.run_computer_ip_;
    l_pre.params.run_time =
        chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_server_task_info.run_time_);
    l_pre.params.end_time =
        chrono::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_server_task_info.end_time_);
    l_pre.params.log_path  = l_server_task_info.log_path_.generic_string();

    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_pre.params.id        = id;
    l_conn(l_pre);
  }
}
void sql_com<doodle::server_task_info>::select(
    doodle::conn_ptr &in_ptr, const std::map<std::int64_t, entt::handle> &in_handle, const doodle::registry_ptr &in_reg
) {
  auto &l_conn = *in_ptr;
  tables::server_task_info l_tabl{};
  std::vector<server_task_info> l_works{};
  std::vector<entt::entity> l_entts{};

  /// 选择大小并进行调整内存
  for (auto &&raw :
       l_conn(sqlpp::select(sqlpp::count(l_tabl.entity_id)).from(l_tabl).where(l_tabl.entity_id.is_not_null()))) {
    l_works.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto &row : l_conn(sqlpp::select(
                              l_tabl.entity_id, l_tabl.data, l_tabl.status, l_tabl.name, l_tabl.source_computer,
                              l_tabl.submitter, l_tabl.submit_time, l_tabl.run_computer, l_tabl.run_computer_ip,
                              l_tabl.run_time, l_tabl.end_time, l_tabl.log_path
       )
                              .from(l_tabl)
                              .where(l_tabl.entity_id.is_not_null()))) {
    server_task_info l_u{};
    l_u.name_            = row.name.value();
    l_u.source_computer_ = row.source_computer.value();
    l_u.submitter_       = row.submitter.value();
    l_u.submit_time_     = chrono::time_point_cast<chrono::system_clock::duration>(row.submit_time.value());
    l_u.run_computer_    = row.run_computer.value();
    l_u.run_computer_ip_ = row.run_computer_ip.value();
    l_u.run_time_        = chrono::time_point_cast<chrono::system_clock::duration>(row.run_time.value());
    l_u.end_time_        = chrono::time_point_cast<chrono::system_clock::duration>(row.end_time.value());
    l_u.log_path_        = row.log_path.value();
    auto l_id            = row.entity_id.value();

    if (in_handle.find(l_id) != in_handle.end()) {
      l_works.push_back(l_u);
      l_entts.push_back(in_handle.at(l_id).entity());
    } else {
      default_logger_raw()->error("server_task_info select error id {}", l_id);
    }
  }

  in_reg->insert<server_task_info>(l_entts.begin(), l_entts.end(), l_works.begin());
}
void sql_com<doodle::server_task_info>::destroy(doodle::conn_ptr &in_ptr, const std::vector<std::int64_t> &in_handle) {
  detail::sql_com_destroy<tables::server_task_info>(in_ptr, in_handle);
}
}  // namespace doodle::database_n