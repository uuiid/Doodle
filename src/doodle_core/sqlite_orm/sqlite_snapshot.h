//
// Created by td_main on 2023/11/3.
//

#pragma once
#include <doodle_core/core/core_sql.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/hana.hpp>

#include <entt/entt.hpp>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
namespace doodle::snapshot {

/**
 * 使用反射序列化注册表中组件, 并且保存到sqlite中
 * 保存需要的反射
 * key           | 签名
 * create_table:  void (const conn_ptr& in_conn)
 * begin_save:    std::shared_ptr<void> (const conn_ptr& in_conn)
 * save:          void (const Component& in_com, entt::entity& in_entity,
 * std::shared_ptr<void>& in_pre, const conn_ptr& in_conn)
 * destroy:       void (const std::vector<std::int64_t>&in_vector, const conn_ptr& in_conn)
 *
 * 加载需要的反射:
 *  key           | 签名
 * get_size       std::underlying_type_t<entt::entity> (const conn_ptr& in_conn)
 * begin_load     std::shared_ptr<void> (const conn_ptr& in_conn)
 * load_entt      void (entt::entity& in_entity, std::shared_ptr<void>& in_pre)
 * load_com       void (Component& in_entity, std::shared_ptr<void>& in_pre)
 * has_table      bool (const conn_ptr& in_conn)
 *
 *
 * 如果需要增量保存, 需要使用 class observer_main  类
 */
class sqlite_snapshot {
  struct save_snapshot_t {
    entt::snapshot snapshot_;
    entt::meta_func begin_save_func;
    entt::meta_func save_func;
    entt::meta_func destroy_func;
    // 创建表
    entt::meta_func create_table_func;
    entt::entity entity_{};
    entt::meta_any pre_sql_data_;
    bool is_entity_{false};
    sql_connection_ptr conn_ptr_;
    explicit save_snapshot_t(entt::registry& in_registry, sql_connection_ptr in_conn)
        : snapshot_{in_registry}, conn_ptr_{std::move(in_conn)} {}

    template <typename Component>
    save_snapshot_t& save() {
      set_save_func<Component>();
      if (!create_table_func) return *this;
      if (!begin_save_func) return *this;
      is_entity_ = std::is_same_v<Component, entt::entity>;
      create_table_func.invoke({}, entt::forward_as_meta(conn_ptr_));
      pre_sql_data_ = begin_save_func.invoke({}, entt::forward_as_meta(conn_ptr_));
      snapshot_.get<Component>(*this);
      return *this;
    }
    template <typename Component, typename It>
      requires(!std::is_same_v<Component, entt::entity>)
    save_snapshot_t& save(It first, It last) {
      set_save_func<Component>();
      if (!create_table_func) return *this;
      if (!begin_save_func) return *this;
      if (first == last) return *this;
      is_entity_ = std::is_same_v<Component, entt::entity>;
      create_table_func.invoke({}, entt::forward_as_meta(conn_ptr_));
      pre_sql_data_ = begin_save_func.invoke({}, entt::forward_as_meta(conn_ptr_));
      snapshot_.get<Component>(*this, first, last);
      return *this;
    }
    template <typename Component, typename It>
    save_snapshot_t& destroy(It first, It last) {
      set_save_func<Component>();
      if (!destroy_func) return *this;
      std::vector<std::int64_t> l_vec{};
      for (; first != last; ++first) l_vec.push_back(*first);
      if (l_vec.empty()) return *this;
      destroy_func.invoke({}, l_vec, entt::forward_as_meta(conn_ptr_));
      return *this;
    }

    template <typename Component>
    void set_save_func() {
      auto l_mate       = entt::resolve<Component>();
      begin_save_func   = l_mate.func("begin_save"_hs);
      save_func         = l_mate.func("save"_hs);
      destroy_func      = l_mate.func("destroy"_hs);
      create_table_func = l_mate.func("create_table"_hs);
    }

    inline void operator()(std::underlying_type_t<entt::entity> in_underlying_type) {}
    // 然后是实体和对应组件的循环
    inline void operator()(entt::entity in_entity) {
      entity_ = in_entity;
      if (is_entity_) save_func.invoke(entity_, in_entity, pre_sql_data_, entt::forward_as_meta(conn_ptr_));
    }

    // 组件的加载和保存
    template <typename T>
    void operator()(const T& in_t) {
      save_func.invoke(in_t, entity_, pre_sql_data_, entt::forward_as_meta(conn_ptr_));
    }
  };

  struct load_snapshot_t {
    entt::snapshot_loader loader_;
    entt::meta_func begin_load_func;
    entt::meta_func load_entt_func;
    entt::meta_func load_com_func;
    entt::meta_func has_table_func;
    entt::meta_func get_size_func;
    entt::meta_any result_sql_data_;

    const sql_connection_ptr conn_ptr_;
    explicit load_snapshot_t(entt::registry& in_registry, sql_connection_ptr in_conn)
        : loader_{in_registry}, conn_ptr_{std::move(in_conn)} {}

    template <typename Component>
    load_snapshot_t& load() {
      set_load_func<Component>();
      if (!begin_load_func) return *this;
      if (!has_table_func) return *this;
      if (!has_table_func.invoke({}, entt::forward_as_meta(conn_ptr_)).cast<bool>()) return *this;

      loader_.get<Component>(*this);
      return *this;
    }

    template <typename Component>
    void set_load_func() {
      auto l_mate     = entt::resolve<Component>();
      get_size_func   = l_mate.func("get_size"_hs);
      begin_load_func = l_mate.func("begin_load"_hs);
      load_entt_func  = l_mate.func("load_entt"_hs);
      load_com_func   = l_mate.func("load_com"_hs);
      has_table_func  = l_mate.func("has_table"_hs);
    }
    // load
    inline void operator()(std::underlying_type_t<entt::entity>& in_underlying_type) {
      in_underlying_type =
          get_size_func.invoke({}, entt::forward_as_meta(conn_ptr_)).cast<std::underlying_type_t<entt::entity>>();
      result_sql_data_ = begin_load_func.invoke({}, entt::forward_as_meta(conn_ptr_));
    }
    inline void operator()(entt::entity& in_entity) {
      static entt::entity l_entity{};  // 这块 序列化要加一个实体, 保证正常
      load_entt_func.invoke({}, entt::forward_as_meta(in_entity).as_ref(), result_sql_data_);
    }
    template <typename T>
    void operator()(T& in_t) {
      load_com_func.invoke(in_t, result_sql_data_);
    }
  };

  FSys::path data_path_{};
  entt::registry& registry_;

  void init_base_meta();

  using tx_t = decltype(sqlpp::start_transaction(*std::declval<sql_connection_ptr>()));
  std::shared_ptr<tx_t> tx_{};
  sql_connection_ptr conn_ptr_{};
  std::unique_ptr<save_snapshot_t> save_snapshot_{};

 public:
  explicit sqlite_snapshot(FSys::path in_data_path, entt::registry& in_registry)
      : data_path_(in_data_path.empty() ? FSys::path{":memory:"} : std::move(in_data_path)), registry_{in_registry} {
    init_base_meta();
  }

  virtual ~sqlite_snapshot() = default;
  template <typename... Component>
  void save() {
    database_info l_info{};
    l_info.path_ = data_path_;
    save_snapshot_t l_save{registry_, l_info.get_connection()};
    auto l_tx = sqlpp::start_transaction(*l_save.conn_ptr_);
    (l_save.template save<Component>(), ...);
    l_tx.commit();
  }

  template <typename... Component>
  void load() {
    database_info l_info{};
    l_info.path_ = data_path_;
    load_snapshot_t l_load{registry_, l_info.get_connection_const()};
    (l_load.template load<Component>(), ...);
  }

  template <typename Component, typename It>
  auto destroy(It first, It last) {
    database_info l_info{};
    l_info.path_ = data_path_;
    save_snapshot_t l_save{registry_, l_info.get_connection()};
    auto l_tx = tx_ ? tx_ : std::make_shared<tx_t>(sqlpp::start_transaction(*l_save.conn_ptr_));
    l_save.destroy<Component>(first, last);
    if (!tx_) l_tx->commit();  // 如果是自己创建的事务, 需要自己提交
  }

 private:
  template <typename type_t>
  friend class impl_obs;
  template <typename... Arg_Com>
  friend class observer_main;

  template <typename Component, typename It>
    requires(!std::is_same_v<Component, entt::entity>)
  sqlite_snapshot& save(It first, It last) {
    if (first == last) return *this;
    save_snapshot_->save<Component>(first, last);
    return *this;
  }
  void start_tx() {
    database_info l_info{};
    l_info.path_   = data_path_;
    save_snapshot_ = std::make_unique<save_snapshot_t>(registry_, l_info.get_connection());
    tx_            = std::make_shared<tx_t>(sqlpp::start_transaction(*save_snapshot_->conn_ptr_));
  }
  void commit() {
    if (tx_) tx_->commit();
  }
};

}  // namespace doodle::snapshot