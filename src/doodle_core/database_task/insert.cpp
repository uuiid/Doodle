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
namespace {
/**
 * @brief 组件数据
 */
class com_data {
 public:
  com_data(entt::entity in_entt,
           std::uint32_t in_id,
           std::string in_str)
      : entt_(in_entt),
        com_id(in_id),
        json_data(std::move(in_str)) {}

  entt::entity entt_{};
  std::uint32_t com_id{};
  std::string json_data{};
};
/**
 * @brief 实体数据
 */
class entity_data {
 public:
  entt::entity entt_{};
  std::uint64_t l_id{};
  std::string uuid_data{};
};

}  // namespace
class insert::impl {
 public:
  /**
   * @brief 传入的实体列表
   */
  std::vector<entt::entity> entt_list{};
  /**
   * @brief 实体数据生成
   */
  std::map<entt::entity, std::shared_ptr<entity_data>> main_tabls;
  /**
   * @brief 组件数据生成
   */
  std::vector<com_data> com_tabls;

  std::vector<std::pair<std::int32_t, std::string>> ctx_tabls;

  using boost_strand = boost::asio::strand<decltype(g_thread_pool().pool_)::executor_type>;

  /**
   * @brief boost 无锁保护
   */
  boost_strand strand_{boost::asio::make_strand(g_thread_pool().pool_)};
  /**
   * @brief 原子停止指示
   */
  std::atomic_bool stop{false};
  /**
   * @brief 线程未来数据获取
   */
  std::vector<std::future<void>> futures_;

  /**
   * @brief 在注册表中插入实体
   * @param in_db 传入的插入数据库连接
   */
  void insert_db_entity(sqlpp::sqlite3::connection &in_db) {
    sql::Entity l_tabl{};
    auto l_pre = in_db.prepare(
        sqlpp::insert_into(l_tabl)
            .set(
                l_tabl.uuidData = sqlpp::parameter(l_tabl.uuidData)));

    for (auto &&i : main_tabls) {
      l_pre.params.uuidData = i.second->uuid_data;
      i.second->l_id        = in_db(l_pre);
    }
  }
  /**
   * @brief 在数据库中插入组件
   * @param in_db 传入的插入数据库连接
   */
  void insert_db_com(sqlpp::sqlite3::connection &in_db) {
    sql::ComEntity l_tabl{};
    auto l_pre = in_db.prepare(
        sqlpp::insert_into(
            l_tabl)
            .set(
                l_tabl.jsonData = sqlpp::parameter(l_tabl.jsonData),
                l_tabl.comHash  = sqlpp::parameter(l_tabl.comHash),
                l_tabl.entityId = sqlpp::parameter(l_tabl.entityId)));
    for (auto &&j : com_tabls) {
      l_pre.params.jsonData = j.json_data;
      l_pre.params.comHash  = j.com_id;
      l_pre.params.entityId = main_tabls.at(j.entt_)->l_id;
      auto l_size           = in_db(l_pre);
      DOODLE_LOG_INFO("插入数据 id {}", l_size);
    }
  }

  /**
   * @brief 创建实体数据(多线程)
   */
  void create_entt_data() {
    main_tabls = entt_list |
                 ranges::view::transform([](const entt::entity &in) {
                   auto l_i   = std::make_shared<entity_data>();
                   l_i->entt_ = in;
                   return std::make_pair(in, l_i);
                 }) |
                 ranges::to<std::map<entt::entity, std::shared_ptr<entity_data>>>();
    ranges::for_each(main_tabls, [this](decltype(main_tabls)::value_type &in) {
      futures_.emplace_back(
          boost::asio::post(
              g_thread_pool().pool_,
              std::packaged_task<void()>{
                  [=]() {
                    auto l_h = entt::handle{*g_reg(), in.second->entt_};
                    in.second->uuid_data =
                        boost::uuids::to_string(l_h.get<database>().uuid());
                  }}));
    });
  }

  //#define Type_T doodle::project
  /**
   * @brief 创建组件数据
   * @tparam Type_T 组件类型
   */
  template <typename Type_T>
  void _create_com_data_() {
    ranges::for_each(entt_list, [this](const entt::entity &in) {
      futures_.emplace_back(
          boost::asio::post(
              strand_,
              std::packaged_task<void()>{
                  [=]() {
                    auto l_h = entt::handle{*g_reg(), in};
                    if (l_h.all_of<Type_T>()) {
                      nlohmann::json l_j{};
                      l_j = l_h.get<Type_T>();
                      com_tabls.emplace_back(in, entt::type_id<Type_T>().hash(), l_j.dump());
                    }
                  }}));
    });
  }
  template <typename... Type_T>
  void create_com_data() {
    (_create_com_data_<Type_T>(), ...);
  }

  /**
   * @brief 从主线程开始调用的函数
   */
  void th_insert() {
    create_entt_data();
    create_com_data<doodle::project,
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
                    doodle::redirection_path_info>();

    for (auto &f : futures_) {
      f.get();
    }
    auto l_comm = core_sql::Get().get_connection(g_reg()->ctx().at<database_info>().path_);
    insert_db_entity(*l_comm);
    insert_db_com(*l_comm);
  }
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
