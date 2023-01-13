#include "work_task.h"

#include <doodle_core/generate/core/sql_sql.h>

#include <sqlpp11/ppgen.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {
namespace sql = doodle_database;
void sql_com<doodle::work_task_info, false>::create_table(conn_ptr& in_ptr) {
  static std::once_flag l_f{};
  std::call_once(l_f, [&]() {
    auto l_f = cmrc::DoodleLibResource::get_filesystem().open("core/sql_file.sql");

    in_ptr->execute(std::string{l_f.cbegin(), l_f.cend()});
  });
}

namespace {
struct insert_id_time {
  using time_type = chrono::time_point<doodle::chrono::system_clock, doodle::chrono::microseconds>;
  explicit insert_id_time(std::uint64_t in_id, time_type in_time_type) : l_id(in_id), l_time(in_time_type){};
  std::uint64_t l_id{};
  time_type l_time{};
};
}  // namespace

void sql_com<doodle::work_task_info, false>::insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_handle) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  std::vector<insert_id_time> l_time_v{};
  {
    sql::WorkTaskInfo l_tabl{};

    auto l_pre = l_conn.prepare(sqlpp::insert_into(l_tabl).set(
        l_tabl.userId = sqlpp::parameter(l_tabl.userId), l_tabl.taskName = sqlpp::parameter(l_tabl.taskName),
        l_tabl.region = sqlpp::parameter(l_tabl.region), l_tabl.abstract = sqlpp::parameter(l_tabl.abstract),
        l_tabl.entityId = sqlpp::parameter(l_tabl.entityId)
    ));

    for (auto& l_h : in_handle) {
      auto& l_work          = l_h.get<work_task_info>();
      l_pre.params.region   = l_work.region;
      l_pre.params.taskName = l_work.task_name;
      l_pre.params.abstract = l_work.abstract;
      if (auto l_user_h = l_work.user_ref.user_attr(); l_user_h.all_of<database>()) {
        l_pre.params.userId = uuids::to_string(l_user_h.get<database>().uuid());
      }
      auto l_r = l_conn(l_pre);
      l_time_v.emplace_back(
          l_r, chrono_ns::round<insert_id_time::time_type::duration>(chrono_ns::to_sys_point(l_work.time))
      );
    }
  }

  {
    sql::TimeWarp l_tabl{};

    auto l_pre = l_conn.prepare(sqlpp::insert_into(l_tabl).set(
        l_tabl.timeValue = sqlpp::parameter(l_tabl.timeValue), l_tabl.parentId = sqlpp::parameter(l_tabl.parentId)
    ));

    for (auto& l_i : l_time_v) {
      l_pre.params.parentId  = boost::numeric_cast<std::int64_t>(l_i.l_id);
      l_pre.params.timeValue = l_i.l_time;
      l_conn(l_pre);
    }
  }
}
void sql_com<doodle::work_task_info, false>::update(conn_ptr& in_ptr, const entt::handle& in_handle) {}
void sql_com<doodle::work_task_info, false>::select(conn_ptr& in_ptr, entt::handle& in_handle) {}
void sql_com<doodle::work_task_info, false>::destroy(conn_ptr& in_ptr, const entt::handle& in_handle) {}

}  // namespace doodle::database_n