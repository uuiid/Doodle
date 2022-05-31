//
// Created by TD on 2022/5/30.
//

#include "select.h"
#include <thread_pool/process_message.h>
#include <doodle_core/core/doodle_lib.h>
#include <thread_pool/thread_pool.h>

#include <core/core_sql.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/ppgen.h>

#include <range/v3/range.hpp>
#include <range/v3/range_for.hpp>
#include <range/v3/all.hpp>


namespace doodle {
namespace database_n {
class select::impl {
 public:
  FSys::path project;
  bool only_ctx;
  std::future<void> result;

  void add_ctx_table(sqlpp::sqlite3::connection& in_conn) {
    in_conn.execute(R"(
create table entity
(
    id          integer auto_increment
        constraint entity_pk
            primary key,
    uuid_data   text,
    update_time datetime default CURRENT_TIMESTAMP not null

);
)");
    in_conn.execute(R"(
create index if not exists context_index_id
    on context (id);
)");
  }

  void add_entity_table(sqlpp::sqlite3::connection& in_conn) {
    in_conn.execute(R"(
create table entity
(
    id          integer auto_increment
        constraint entity_pk
            primary key,
    uuid_data   text,
    update_time datetime default CURRENT_TIMESTAMP not null

);
)");
    in_conn.execute(R"(
create index if not exists entity_index
    on entity (id);
)");
  }

  void add_component_table(sqlpp::sqlite3::connection& in_conn,
                           const std::string in_com_name) {
    in_conn.execute(fmt::format(R"(
create table com_{}
(
    id        integer auto_increment
        constraint entity_pk
            primary key,
    entity_id integer
        constraint entity_id_ref references entity (id),
    json_data text
);
)",
                                in_com_name));

    in_conn.execute(fmt::format(R"(
create index if not exists com_{0}_index
    on com_{0} (id);
)",
                                in_com_name));
    in_conn.execute(fmt::format(R"(
create trigger UpdataLastTime_{0} AFTER UPDATE OF json_data
    ON com_{0}
begin
    update entity set update_time =CURRENT_TIMESTAMP where id = old.entity_id;
end;
)",
                                in_com_name));
  }

  std::vector<std::string> get_tables(sqlpp::sqlite3::connection& in_conn) {
    auto statement = sqlpp::custom_query(
                         sqlpp::verbatim(
                             R"(SELECT name FROM main WHERE type='table' ORDER BY name ;)"))
                         .with_result_type_of(
                             sqlpp::select(sqlpp::value("").as(sqlpp::alias::a)));

    std::vector<std::string> l_r{};
    for (auto& row : in_conn(statement)) {
      l_r.emplace_back(row.a.value());
    }
    return l_r;
  }
  template <class... Table_Class>

  void auto_create_component_table() {
    std::vector<std::string> l_com_names{entt::type_name<Table_Class>().value()...};

    ranges::for_each(l_com_names,[](std::string& in_name){
      ;
    });

  };
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
