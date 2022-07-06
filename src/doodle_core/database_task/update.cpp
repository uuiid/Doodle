//
// Created by TD on 2022/5/30.
//

#include "update.h"

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
#include <database_task/details/update_ctx.h>

namespace doodle::database_n {
namespace sql = doodle_database;

class update_data::impl {
 private:
  std::vector<std::future<void>> futures_;

 public:
  using com_data = details::com_data;
  std::vector<entt::entity> entt_list{};

  std::vector<com_data> com_tabls;
  std::map<entt::entity, std::int64_t> main_tabls;
  using boost_strand = boost::asio::strand<decltype(g_thread_pool().pool_)::executor_type>;
  ///@brief boost 无锁保护
  boost_strand strand_{boost::asio::make_strand(g_thread_pool().pool_)};
  //#define Type_T doodle::project

  /// \brief 最终的ji结果
  std::future<void> future_;
  ///@brief 原子停止指示
  std::atomic_bool stop{false};

  std::size_t size;

  void updata_db_table(sqlpp::sqlite3::connection &in_db) {
    auto [l_main_v, l_s_v] = details::get_version(in_db);
    if (l_main_v <= 3 && l_s_v <= 4) {
      details::add_entity_table(in_db);
      details::add_ctx_table(in_db);
      details::add_component_table(in_db);
      details::set_version(in_db);
    }
  }

  void updata_db(sqlpp::sqlite3::connection &in_db) {
    sql::ComEntity l_tabl{};
    auto l_pre = in_db.prepare(
        sqlpp::update(l_tabl)
            .set(l_tabl.jsonData = sqlpp::parameter(l_tabl.jsonData))
            .where(l_tabl.entityId == sqlpp::parameter(l_tabl.entityId) &&
                   l_tabl.comHash == sqlpp::parameter(l_tabl.comHash)));
    for (auto &&i : com_tabls) {
      if (stop)
        return;
      l_pre.params.jsonData = i.json_data;
      l_pre.params.comHash  = i.com_id;
      l_pre.params.entityId = main_tabls[i.entt_];
      auto l_s              = in_db(l_pre);
      DOODLE_LOG_INFO("更新数据库id {}", l_s);
      g_reg()->ctx().emplace<process_message>().progress_step({1, size * 2});
    }
  }

  void create_entt_data() {
    main_tabls = entt_list |
                 ranges::view::transform([](const entt::entity &in) {
                   return std::make_pair(in, g_reg()->get<database>(in).get_id());
                 }) |
                 ranges::to<std::map<entt::entity, std::int64_t>>();
  }

  template <typename Type_T>
  void _create_com_data_(std::size_t in_size) {
    ranges::for_each(entt_list, [this, in_size](const entt::entity &in_) {
      if (stop)
        return;
      futures_.emplace_back(
          boost::asio::post(
              strand_,
              std::packaged_task<void()>{
                  [=]() {
                    if (stop)
                      return;
                    auto l_h = entt::handle{*g_reg(), in_};
                    if (l_h.all_of<Type_T>()) {
                      auto l_json = nlohmann::json{};
                      l_json      = l_h.get<Type_T>();
                      com_tabls.emplace_back(in_,
                                             entt::type_id<Type_T>().hash(),
                                             l_json.dump());
                    }
                    g_reg()->ctx().emplace<process_message>().progress_step({1, in_size * size * 2});
                  }}));
    });
  }

  template <typename... Type_T>
  void create_com_data() {
    auto l_size = sizeof...(Type_T);
    (_create_com_data_<Type_T>(l_size), ...);
  }

  void th_updata() {
    g_reg()->ctx().emplace<process_message>().message("创建实体数据");
    create_entt_data();
#include "details/macro.h"
    g_reg()->ctx().emplace<process_message>().message("组件数据...");
    create_com_data<DOODLE_SQLITE_TYPE>();

    g_reg()->ctx().emplace<process_message>().message("完成数据线程准备");
    for (auto &f : futures_) {
      f.get();
    }
    auto l_comm = core_sql::Get().get_connection(g_reg()->ctx().at<database_info>().path_);
    auto l_tx   = sqlpp::start_transaction(*l_comm);

    g_reg()->ctx().emplace<process_message>().message("检查数据库架构");
    updata_db_table(*l_comm);

    g_reg()->ctx().emplace<process_message>().message("组件更新...");
    updata_db(*l_comm);
    g_reg()->ctx().emplace<process_message>().message("更新上下文...");
    doodle::database_n::details::update_ctx::ctx(*g_reg(), *l_comm);
    g_reg()->ctx().emplace<process_message>().message("完成");
    l_tx.commit();
  }
};
update_data::update_data(const std::vector<entt::entity> &in_data)
    : p_i(std::make_unique<impl>()) {
  p_i->entt_list = in_data;
  p_i->size      = p_i->entt_list.size();
}
update_data::~update_data() = default;

void update_data::init() {
  auto &k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("插入数据");
  k_msg.set_state(k_msg.run);
  p_i->future_ = g_thread_pool().enqueue([this]() {
    p_i->th_updata();
  });
}
void update_data::succeeded() {
  g_reg()->ctx().erase<process_message>();
}
void update_data::failed() {
  g_reg()->ctx().erase<process_message>();
}
void update_data::aborted() {
  g_reg()->ctx().erase<process_message>();
  p_i->stop = true;
}
void update_data::update(
    chrono::duration<chrono::system_clock::rep,
                     chrono::system_clock::period>,
    void *data) {
  switch (p_i->future_.wait_for(0ns)) {
    case std::future_status::ready: {
      try {
        p_i->future_.get();
        this->succeed();
      } catch (const doodle_error &error) {
        DOODLE_LOG_ERROR(error.what());
        this->fail();
        throw;
      }
    } break;
    default:
      break;
  }
}
}  // namespace doodle::database_n
