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

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/ppgen.h>

#include <range/v3/range.hpp>
#include <range/v3/range_for.hpp>
#include <range/v3/all.hpp>

#include <boost/asio.hpp>

SQLPP_DECLARE_TABLE(
    (doodle_info),
    (version_major, int, SQLPP_NULL)(version_minor, int, SQLPP_NULL));

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

  registry_ptr local_reg{std::make_shared<entt::registry>()};

  static void add_ctx_table(sqlpp::sqlite3::connection& in_conn) {
    in_conn.execute(std::string{create_ctx_table});
    in_conn.execute(std::string{create_ctx_table_index});
    in_conn.execute(std::string{create_ctx_table_unique});
  }

  static void add_entity_table(sqlpp::sqlite3::connection& in_conn) {
    in_conn.execute(std::string{create_entity_table});
    in_conn.execute(std::string{create_entity_table_index});
  }

  static void add_component_table(sqlpp::sqlite3::connection& in_conn) {
    in_conn.execute(std::string{create_com_table});
    in_conn.execute(std::string{create_com_table_index_id});
    in_conn.execute(std::string{create_com_table_index_hash});
    in_conn.execute(std::string{create_com_table_trigger});
  }

  std::tuple<std::uint32_t, std::uint32_t> get_version(
      sqlpp::sqlite3::connection& in_conn) {
    doodle_info::doodle_info l_info{};

    for (auto&& row : in_conn(
             sqlpp::select(all_of(l_info)).from(l_info).unconditionally())) {
      return std::make_tuple(boost::numeric_cast<std::uint32_t>(row.version_major.value()),
                             boost::numeric_cast<std::uint32_t>(row.version_minor.value()));
    }
    chick_true<doodle_error>(false,
                             DOODLE_LOC,
                             "无法检查到数据库");
    return {};
  }

  static void set_version(sqlpp::sqlite3::connection& in_conn) {
    doodle_info::doodle_info l_info{};

    in_conn(sqlpp::update(l_info).unconditionally().set(
        l_info.version_major = version::version_major,
        l_info.version_minor = version::version_minor));
  }

  void up_data() {
    auto k_con             = core_sql::Get().get_connection(project);
    auto [l_main_v, l_s_v] = get_version(*k_con);
    if (l_main_v <= 3 && l_s_v <= 4) {
      add_entity_table(*k_con);
      add_ctx_table(*k_con);
      add_component_table(*k_con);
    }
    if (l_main_v < version::version_major || l_s_v < version::version_minor) {
      set_version(*k_con);
    }
  }

  void create_db() {
    if (!FSys::exists(project)) {
      if (!FSys::exists(project.parent_path()))
        FSys::create_directories(project.parent_path());

      auto k_con = core_sql::Get().get_connection(project);
      add_entity_table(*k_con);
      add_ctx_table(*k_con);
      add_component_table(*k_con);
      set_version(*k_con);
    }
  }

  void select_old(entt::registry& in_reg, sqlpp::sqlite3::connection& in_conn) {
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
                  }});
      results.emplace_back(l_fun.share());
      if (stop)
        return;
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
                chick_true<doodle_error>(
                    l_h.valid(),
                    DOODLE_LOC,
                    "无效的实体");
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
               l_e,
               &in_reg,
               this]() {
                if (stop)
                  return;

                entt::handle l_h{in_reg, l_e};
                chick_true<doodle_error>(
                    l_h.valid(), DOODLE_LOC,
                    "失效的实体");
                if (l_h.get<database>() == in_json)
                  return;

                l_h.emplace<database>(in_json);
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
  p_i->results.emplace_back(
      g_thread_pool().enqueue([this]() {
                       this->th_run();
                     })
          .share());
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
  if (!p_i->results.empty()) {
    p_i->size_ = std::max(p_i->size_, p_i->results.size());
    ranges::remove_if(p_i->results,
                      [this](const std::shared_future<void>& in_r) {
                        if (in_r.wait_for(0ns) == std::future_status::ready) {
                          g_reg()->ctx().emplace<process_message>().progress_step({1, p_i->size_});
                          try {
                            in_r.get();
                          } catch (const doodle_error& error) {
                            DOODLE_LOG_ERROR(error.what());
                            this->p_i->stop = true;
                            this->fail();
                            throw;
                          };
                          return true;
                        }
                        return false;
                      });

  } else {
    std::swap(g_reg(), p_i->local_reg);
    this->succeed();
  }
}

void select::th_run() {
  auto l_k_con = core_sql::Get().get_connection_const(p_i->project);
  this->p_i->select_old(*p_i->local_reg, *l_k_con);

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

}  // namespace doodle::database_n
