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

class insert::impl {
 public:
  std::vector<entt::entity> entt_list{};

  std::vector<std::pair<std::int32_t, std::string>> entity_tabls;
  std::vector<std::pair<std::int32_t, std::string>> com_tabls;

  using boost_strand = boost::asio::strand<decltype(g_thread_pool().pool_)::executor_type>;

  std::vector<boost_strand> strands_{};
  std::atomic_bool stop{false};
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
