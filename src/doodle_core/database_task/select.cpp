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
    in_conn.execute(R"(
create table com_entity
(
    id        integer auto_increment
        constraint entity_pk
            primary key,
    entity_id integer
        constraint entity_id_ref references entity (id),
    com_hash integer,
    json_data text
);
)");

    in_conn.execute(R"(
create index if not exists com_entity_index
    on com_entity (id);
)");
    in_conn.execute(R"(
create index if not exists com_entity_index_hash
    on com_entity (com_hash);
)");
    in_conn.execute(R"(
create trigger UpdataLastTime_ AFTER UPDATE OF json_data
    ON com_entity
begin
    update entity set update_time =CURRENT_TIMESTAMP where id = old.entity_id;
end;
)");
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
