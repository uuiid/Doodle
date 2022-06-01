//
// Created by TD on 2022/5/30.
//

#include "insert.h"
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
#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/database_task/sql_file.h>

#include <doodle_core/generate/core/sql_sql.h>
#include <doodle_core/generate/core/metadatatab_sql.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/ppgen.h>

#include <range/v3/range.hpp>
#include <range/v3/range_for.hpp>
#include <range/v3/all.hpp>

#include <boost/asio.hpp>
namespace doodle::database_n {
namespace sql = doodle_database;

class insert::impl {
 public:
  std::vector<entt::entity> entt_list{};

  class com_data {
   public:
    std::int64_t com_id;
    std::string json_data;
  };

  class entity_data {
   public:
    entt::entity entt_;
    std::uint64_t l_id;
    std::string uuid_data;
    std::vector<com_data> l_coms;
  };

  std::vector<std::shared_ptr<entity_data>> main_tabls;
  std::vector<std::pair<std::int32_t, std::string>> ctx_tabls;

  using boost_strand = boost::asio::strand<decltype(g_thread_pool().pool_)::executor_type>;

  std::vector<boost_strand> strands_{};
  std::atomic_bool stop{false};

  void insert_db(sqlpp::sqlite3::connection &in_db) {
    {
      sql::Entity l_tabl{};
      auto l_pre = in_db.prepare(
          sqlpp::insert_into(l_tabl)
              .set(
                  l_tabl.uuidData = sqlpp::parameter(l_tabl.uuidData)));

      for (auto &&i : main_tabls) {
        l_pre.params.uuidData = i->uuid_data;
        i->l_id                = in_db(l_pre);
      }
    }
    {
      sql::ComEntity l_tabl{};
      auto l_pre = in_db.prepare(
          sqlpp::insert_into(
              l_tabl)
              .set(
                  l_tabl.jsonData = sqlpp::parameter(l_tabl.jsonData),
                  l_tabl.comHash  = sqlpp::parameter(l_tabl.comHash),
                  l_tabl.entityId = sqlpp::parameter(l_tabl.entityId)));
      for (auto &&i : main_tabls) {
        for (auto &&j : i->l_coms) {
          l_pre.params.jsonData = j.json_data;
          l_pre.params.comHash  = j.com_id;
          l_pre.params.entityId = i->l_id;
          auto l_size           = in_db(l_pre);
          DOODLE_LOG_INFO("插入数据 id {}", l_size);
        }
      }
    }
  }

  void create_entt_data() {
  }
  void create_com_data() {}
};
insert::insert(const std::vector<entt::entity> &in_inster)
    : p_i(std::make_unique<impl>()) {
  p_i->entt_list = in_inster;
}
insert::~insert() = default;
void insert::init() {
  ;
}
void insert::succeeded() {
}
void insert::failed() {
}
void insert::aborted() {
}
void insert::update(
    chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>,
    void *data) {
}

}  // namespace doodle::database_n
