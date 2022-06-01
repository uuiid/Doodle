//
// Created by TD on 2022/5/30.
//

#include "delete_data.h"

#include <doodle_core/thread_pool/process_message.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/thread_pool/thread_pool.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/core/core_sql.h>

#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/organization.h>
#include <doodle_core/metadata/redirection_path_info.h>

#include <doodle_core/generate/core/sql_sql.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>

#include <range/v3/all.hpp>

#include <database_task/details/com_data.h>

namespace doodle {
namespace database_n {
namespace sql = doodle_database;

class delete_data::impl {
 public:
  /// \brief 实体列表
  std::vector<entt::entity> entt_list{};
  /// \brief 数据库id列表
  std::vector<std::uint64_t> delete_id_list{};

  void create_id() {
    delete_id_list =
        entt_list |
        ranges::view::transform([](const entt::entity &in) {
          return g_reg()->get<database>(in).get_id();
        }) |
        ranges::to_vector;
  }

  void delete_db(sqlpp::sqlite3::connection &in_db) {
    sql::Entity l_tabl{};
    auto l_pre = in_db.prepare(
        sqlpp::remove_from(l_tabl)
            .where(l_tabl.id == sqlpp::parameter(l_tabl.id)));
    for (auto &&i : delete_id_list) {
      l_pre.params.id = i;
      in_db(l_pre);
    }
  }

  void th_delete() {
    create_id();
    {
      auto l_comm = core_sql::Get().get_connection(g_reg()->ctx().at<database_info>().path_);
      delete_db(*l_comm);
    }
    g_reg()->destroy(entt_list.begin(), entt_list.end());
  }
};
delete_data::delete_data(const std::vector<entt::entity> &in_data)
    : p_i(std::make_unique<impl>()) {
  p_i->entt_list = in_data;
}
delete_data::~delete_data() = default;
void delete_data::init() {
}
void delete_data::succeeded() {
}
void delete_data::failed() {
}
void delete_data::aborted() {
}
void delete_data::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void *data) {
}
}  // namespace database_n
}  // namespace doodle
