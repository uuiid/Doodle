//
// Created by TD on 2022/5/30.
//

#include "select.h"
#include <thread_pool/process_message.h>
#include <doodle_core/core/doodle_lib.h>
#include <thread_pool/thread_pool.h>
#include <logger/logger.h>
#include <core/core_sql.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/ppgen.h>
#include <doodle_core/database_task/sql_file.h>
#include <range/v3/range.hpp>
#include <range/v3/range_for.hpp>
#include <range/v3/all.hpp>

SQLPP_DECLARE_TABLE(
    (doodle_info),
    (version_major, int, SQLPP_NULL)(version_minor, int, SQLPP_NULL));

namespace doodle {
namespace database_n {
class select::impl {
 public:
  /**
   * 数据库的绝对路径
   */
  FSys::path project;
  bool only_ctx{false};
  std::future<void> result;

  void add_ctx_table(sqlpp::sqlite3::connection& in_conn) {
    in_conn.execute(std::string{create_ctx_table});
    in_conn.execute(std::string{create_ctx_table_index});
  }

  void add_entity_table(sqlpp::sqlite3::connection& in_conn) {
    in_conn.execute(std::string{create_entity_table});
    in_conn.execute(std::string{create_entity_table_index});
  }

  void add_component_table(sqlpp::sqlite3::connection& in_conn) {
    in_conn.execute(std::string{create_com_table});
    in_conn.execute(std::string{create_com_table_index_id});
    in_conn.execute(std::string{create_com_table_index_hash});
    in_conn.execute(std::string{create_com_table_trigger});
  }

  std::tuple<std::uint32_t, std::uint32_t> get_version(
      sqlpp::sqlite3::connection& in_conn) {
    doodle_info::doodle_info l_info{};

    for (auto&& row : in_conn(
             sqlpp::select(all_of(l_info)).from(l_info).unconditionally())) {
      return std::make_tuple(boost::numeric_cast<std::uint32_t>(row.version_major.value()),
                             boost::numeric_cast<std::uint32_t>(row.version_minor.value()));
    }
    chick_true<doodle_error>(false,
                             DOODLE_LOC,
                             "无法检查到数据库");
    return {};
  }

  void set_version(sqlpp::sqlite3::connection& in_conn) const {
    doodle_info::doodle_info l_info{};

    in_conn(sqlpp::update(l_info).unconditionally().set(
        l_info.version_major = version::version_major,
        l_info.version_minor = version::version_minor));
  }

  void up_data() {
    auto k_con             = core_sql::Get().get_connection(project);
    auto [l_main_v, l_s_v] = get_version(*k_con);
    if (l_main_v <= 3 && l_s_v <= 4) {
      add_entity_table(*k_con);
      add_ctx_table(*k_con);
      add_component_table(*k_con);
    }
  }
};

select::select(const select::arg& in_arg) : p_i(std::make_unique<impl>()) {
  p_i->project  = in_arg.project_path;
  p_i->only_ctx = in_arg.only_ctx;
}
select::~select() = default;

void select::init() {
  auto& k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("加载数据");
  k_msg.set_state(k_msg.run);
  p_i->result = g_thread_pool().enqueue([this]() {
    if (!p_i->only_ctx) {
      this->select_db();
    }
    this->select_ctx();
  });
}
void select::succeeded() {
}
void select::failed() {
}
void select::aborted() {
}
void select::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
}
void select::select_db() {
}
void select::select_ctx() {
}
bool select::chick_table() {
  return false;
}
void select::update() {
}
}  // namespace database_n
}  // namespace doodle
