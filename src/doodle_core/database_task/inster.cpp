//
// Created by TD on 2022/5/30.
//

#include "inster.h"
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

class inster::impl {
 public:
  std::vector<entt::entity> entt_list{};
  registry_ptr local_reg{std::make_shared<entt::registry>()};

  template <typename Type>
  void _copy_com_() {
    auto l_v = g_reg()->view<Type>();
    local_reg->insert<Type>(l_v.data(), l_v.data() + l_v.size(),
                            l_v.raw(), l_v.raw() + l_v.size());
  }
};
inster::inster(const std::vector<entt::entity> &in_inster)
    : p_i(std::make_unique<impl>()) {
  p_i->entt_list = in_inster;
}
inster::~inster() = default;
void inster::init() {
  auto &&l_reg = *g_reg();
  p_i->local_reg->assign(l_reg.data(), l_reg.data() + l_reg.size(), l_reg.released());

  auto l_v = g_reg()->view<doodle::project>();
  std::vector<doodle::project> l_lsit{};

  p_i->local_reg->insert<doodle::project>(
      l_reg.data(), l_reg.data() + l_reg.size(),
      l_lsit.begin());
}
void inster::succeeded() {
}
void inster::failed() {
}
void inster::aborted() {
}
void inster::update(
    chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>,
    void *data) {
}

}  // namespace doodle::database_n
