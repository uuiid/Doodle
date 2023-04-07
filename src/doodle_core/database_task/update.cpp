//
// Created by TD on 2022/5/30.
//

#include "update.h"

#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/detail/time_point_info.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_task.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>

#include <database_task/details/com_data.h>
#include <database_task/details/update_ctx.h>
#include <range/v3/all.hpp>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {

class update_data::impl {
 public:
  std::vector<std::future<void>> futures_;
  using com_data = details::com_data;
  std::vector<entt::entity> entt_list{};

  std::vector<com_data> com_tabls;
  std::map<entt::entity, std::int64_t> main_tabls;
  using boost_strand = boost::asio::strand<std::decay_t<decltype(g_thread())>::executor_type>;
  ///@brief boost 无锁保护
  boost_strand strand_{boost::asio::make_strand(g_thread())};
  // #define Type_T doodle::project

  /// \brief 最终的ji结果
  std::future<void> future_;
  ///@brief 原子停止指示
  std::atomic_bool stop{false};

  std::size_t size;

  void updata_db_table(sqlpp::sqlite3::connection &in_db) {
    auto [l_main_v, l_s_v] = details::get_version(in_db);
    if (l_main_v <= 3 && l_s_v <= 4) {
      details::db_compatible::add_entity_table(in_db);
      details::db_compatible::add_ctx_table(in_db);
      details::db_compatible::add_component_table(in_db);
      details::db_compatible::set_version(in_db);
    }
    doodle::database_n::details::db_compatible::delete_metadatatab_table(in_db);
  }

  void updata_db(sqlpp::sqlite3::connection &in_db) {}

  void create_entt_data() {
    main_tabls = entt_list | ranges::views::transform([](const entt::entity &in) {
                   return std::make_pair(in, g_reg()->get<database>(in).get_id());
                 }) |
                 ranges::to<std::map<entt::entity, std::int64_t>>();
  }

  template <typename Type_T>
  void _create_com_data_(std::size_t in_size) {
    ranges::for_each(entt_list, [this, in_size](const entt::entity &in_) {
      if (stop) return;
      futures_.emplace_back(
          boost::asio::post(strand_, std::packaged_task<void()>{[=]() {
                              if (stop) return;
                              auto l_h = entt::handle{*g_reg(), in_};
                              if (l_h.all_of<Type_T>()) {
                                auto l_json = nlohmann::json{};
                                l_json      = l_h.get<Type_T>();
                                com_tabls.emplace_back(in_, entt::type_id<Type_T>().hash(), l_json.dump());
                              }
                              g_reg()->ctx().emplace<process_message>().progress_step({1, in_size * size * 2});
                            }})
      );
    });
  }

  template <typename... Type_T>
  void create_com_data() {
    auto l_size = sizeof...(Type_T);
    (_create_com_data_<Type_T>(l_size), ...);
  }
};
update_data::update_data(const std::vector<entt::entity> &in_data) : p_i(std::make_unique<impl>()) {
  p_i->entt_list = in_data;
  p_i->size      = p_i->entt_list.size();
}
update_data::update_data() : p_i(std::make_unique<impl>()) {}

update_data::~update_data() = default;

void update_data::operator()(
    entt::registry &in_registry, const std::vector<entt::entity> &in_update_data, conn_ptr &in_connect
) {
  p_i->entt_list = in_update_data;
  p_i->size      = p_i->entt_list.size();

  g_reg()->ctx().emplace<process_message>().message("创建实体数据");
  p_i->create_entt_data();
#include "details/macro.h"
  g_reg()->ctx().emplace<process_message>().message("组件数据...");
  p_i->create_com_data<DOODLE_SQLITE_TYPE>();

  g_reg()->ctx().emplace<process_message>().message("完成数据线程准备");
  for (auto &f : p_i->futures_) {
    f.get();
  }
  if (p_i->futures_.empty()) return;

  g_reg()->ctx().emplace<process_message>().message("检查数据库架构");
  p_i->updata_db_table(*in_connect);

  g_reg()->ctx().emplace<process_message>().message("组件更新...");
  p_i->updata_db(*in_connect);
  g_reg()->ctx().emplace<process_message>().message("更新上下文...");
  doodle::database_n::details::update_ctx::ctx(*g_reg(), *in_connect);
  g_reg()->ctx().emplace<process_message>().message("完成");
}
}  // namespace doodle::database_n
