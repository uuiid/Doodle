//
// Created by TD on 2022/5/30.
//

#include "select.h"
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
#include <doodle_core/lib_warp/enum_template_tool.h>
#include <doodle_core/database_task/details/update_ctx.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/ppgen.h>

#include <range/v3/range.hpp>
#include <range/v3/range_for.hpp>
#include <range/v3/all.hpp>

#include <boost/asio.hpp>

namespace doodle::database_n {
namespace sql = doodle_database;
class select::impl {
 public:
  using boost_strand = boost::asio::strand<decltype(g_thread_pool().pool_)::executor_type>;
  /**
   * 数据库的绝对路径
   */
  FSys::path project;
  bool only_ctx{false};
  std::future<void> result;
  std::vector<std::shared_future<void>> results;
  std::atomic_bool stop{false};
  boost_strand
      strand_{boost::asio::make_strand(g_thread_pool().pool_)};
  std::vector<boost_strand> strands_{};
  std::size_t size_{10};

  registry_ptr local_reg{g_reg()};

  void select_old(entt::registry& in_reg, sqlpp::sqlite3::connection& in_conn) {
    if (auto [l_v, l_i] = doodle::database_n::details::get_version(in_conn);
        l_v == 3 && l_i >= 4) {
      Metadatatab l_metadatatab{};

      for (auto&& row : in_conn(sqlpp::select(sqlpp::all_of(l_metadatatab))
                                    .from(l_metadatatab)
                                    .unconditionally())) {
        auto l_e = num_to_enum<entt::entity>(row.id.value());
        if (!in_reg.valid(l_e)) l_e = in_reg.create(l_e);

        auto l_fun =
            boost::asio::post(
                strand_,
                std::packaged_task<void()>{
                    [l_e,
                     in_str  = row.userData.value(),
                     in_uuid = row.uuidData.value(),
                     &in_reg,
                     this]() {
                      if (stop)
                        return;
                      entt::handle l_h{in_reg, l_e};
                      l_h.emplace<database>(in_uuid).set_id(
                          enum_to_num(l_e));
                      auto k_json = nlohmann::json::parse(in_str);
                      entt_tool::load_comm<doodle::project,
                                           doodle::episodes,
                                           doodle::shot,
                                           doodle::season,
                                           doodle::assets,
                                           doodle::assets_file,
                                           doodle::time_point_wrap,
                                           doodle::comment,
                                           doodle::project_config::base_config,
                                           doodle::image_icon,
                                           doodle::importance,
                                           doodle::organization_list,
                                           doodle::redirection_path_info>(l_h, k_json);
                      database::save(l_h);
                    }});
        results.emplace_back(l_fun.share());
        if (stop)
          return;
      }
      auto l_fun =
          boost::asio::post(
              strand_,
              std::packaged_task<void()>{
                  [this]() {
                    auto l_view = local_reg->view<doodle::project>();
                    if (!l_view.empty()) {
                      auto l_h                               = entt::handle{*local_reg, l_view.front()};
                      local_reg->ctx().at<doodle::project>() = l_h.get<doodle::project>();
                      local_reg->ctx().at<doodle::project_config::base_config>() =
                          l_h.any_of<doodle::project_config::base_config>()
                              ? l_h.get<doodle::project_config::base_config>()
                              : doodle::project_config::base_config{};
                    }
                  }});
      results.emplace_back(l_fun.share());
    }
  }

  template <typename Type>
  void _select_com_(entt::registry& in_reg, sqlpp::sqlite3::connection& in_conn) {
    sql::ComEntity l_com_entity{};

    auto&& l_s = strands_.emplace_back(boost::asio::make_strand(g_thread_pool().pool_));
    for (auto&& row : in_conn(
             sqlpp::select(
                 l_com_entity.entityId,
                 l_com_entity.jsonData)
                 .from(l_com_entity)
                 .where(
                     l_com_entity.comHash == entt::type_id<Type>().hash()))) {
      if (stop)
        return;
      auto l_fut = boost::asio::post(
          l_s,
          std::packaged_task<void()>{
              [in_json = row.jsonData.value(),
               in_id   = row.entityId.value(),
               &in_reg,
               this]() {
                if (stop)
                  return;
                entt::entity l_e = num_to_enum<entt::entity>(in_id);
                entt::handle l_h{in_reg, l_e};
                if (!l_h.valid()) {
                  DOODLE_LOG_ERROR("无效的实体 {}", in_id);
                  database::delete_(l_h);
                }

                auto l_json = nlohmann::json::parse(in_json);
                l_h.emplace_or_replace<Type>(std::move(l_json.template get<Type>()));
              }});

      results.emplace_back(l_fut.share());
    }
  }
  template <typename... Type>
  void select_com(entt::registry& in_reg, sqlpp::sqlite3::connection& in_conn) {
    (_select_com_<Type>(in_reg, in_conn), ...);
  }

  void _select_ctx_(entt::registry& in_reg,
                    sqlpp::sqlite3::connection& in_conn,
                    const std::map<std::uint32_t,
                                   std::function<
                                       void(entt::registry& in_reg, const std::string& in_str)>>& in_fun_list) {
    sql::Context l_context{};

    for (auto&& row : in_conn(
             sqlpp::select(l_context.comHash,
                           l_context.jsonData)
                 .from(l_context)
                 .unconditionally())) {
      if (stop)
        return;
      if (auto l_f = in_fun_list.find(row.comHash.value());
          l_f != in_fun_list.end()) {
        in_fun_list.at(row.comHash.value())(in_reg, row.jsonData.value());
      }
    }
  }
  template <typename... Type>
  void select_ctx(entt::registry& in_reg,
                  sqlpp::sqlite3::connection& in_conn) {
    std::map<std::uint32_t,
             std::function<void(entt::registry & in_reg, const std::string& in_str)>>
        l_fun{
            std::make_pair(entt::type_id<Type>().hash(),
                           [](entt::registry& in_reg, const std::string& in_str) {
                             auto l_json = nlohmann::json::parse(in_str);
                             if (in_reg.ctx().template contains<Type>())
                               in_reg.ctx().template erase<Type>();

                             in_reg.ctx().template emplace<Type>(
                                 std::move(l_json.get<Type>()));
                           })...};

    _select_ctx_(in_reg, in_conn, l_fun);
  }

  void select_entt(entt::registry& in_reg,
                   sqlpp::sqlite3::connection& in_conn) {
    sql::Entity l_entity{};

    for (auto& row : in_conn(sqlpp::select(sqlpp::all_of(l_entity))
                                 .from(l_entity)
                                 .unconditionally())) {
      if (stop)
        return;
      auto l_e = num_to_enum<entt::entity>(row.id.value());
      if (!in_reg.valid(l_e)) l_e = in_reg.create(l_e);

      auto l_fut = boost::asio::post(
          strand_,
          std::packaged_task<void()>{
              [in_json = row.uuidData.value(),
               in_id   = row.id,
               l_e,
               &in_reg,
               this]() {
                if (stop)
                  return;

                entt::handle l_h{in_reg, l_e};
                chick_true<doodle_error>(
                    l_h.valid(), DOODLE_LOC,
                    "失效的实体");
                if (l_h.any_of<database>()) {
                  l_h.remove<data_status_save>();
                } else
                  l_h.emplace<database>(in_json).set_id(in_id);
              }});

      results.emplace_back(l_fut.share());
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
    this->th_run();
  });
}
void select::succeeded() {
  g_reg()->ctx().erase<process_message>();
}
void select::failed() {
  g_reg()->ctx().erase<process_message>();
}
void select::aborted() {
  g_reg()->ctx().erase<process_message>();
  p_i->stop = true;
}
void select::update(chrono::duration<chrono::system_clock::rep,
                                     chrono::system_clock::period>,
                    void* data) {
  if (p_i->result.valid()) {
    switch (p_i->result.wait_for(0ns)) {
      case std::future_status::ready: {
        try {
          p_i->result.get();
        } catch (const doodle_error& error) {
          DOODLE_LOG_ERROR(error.what());
          this->fail();
          throw;
        }
      } break;
      default:
        break;
    }
  } else {
    std::swap(g_reg(), p_i->local_reg);
    this->succeed();
  }
}

void select::th_run() {
  chick_true<doodle_error>(
      FSys::exists(p_i->project),
      DOODLE_LOC,
      "数据库不存在 {}", p_i->project);

  auto l_k_con = core_sql::Get().get_connection_const(p_i->project);
  this->p_i->select_old(*p_i->local_reg, *l_k_con);

  /// \brief 等待旧的任务完成
  ranges::for_each(p_i->results, [](const decltype(p_i->results)::value_type& in_) {
    in_.get();
  });

  if (auto [l_v, l_i] = doodle::database_n::details::get_version(*l_k_con);
      l_v >= 3 && l_i > 4) {
    if (!p_i->only_ctx) {
      /// \brief 选中实体
      p_i->select_entt(*p_i->local_reg, *l_k_con);
#include "details/macro.h"
      /// @brief 选中组件
      p_i->select_com<DOODLE_SQLITE_TYPE>(*p_i->local_reg, *l_k_con);
    }
    /// \brief 选中上下文
    {
      p_i->select_ctx<doodle::project,
                      doodle::project_config::base_config>(*p_i->local_reg, *l_k_con);
    }
  }

  /// \brief 等待所有的任务完成
  ranges::for_each(p_i->results, [](const decltype(p_i->results)::value_type& in_) {
    in_.get();
  });

  p_i->local_reg->ctx().at<project>().set_path(p_i->project.parent_path());
}

}  // namespace doodle::database_n
