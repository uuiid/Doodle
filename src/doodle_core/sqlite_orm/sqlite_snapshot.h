//
// Created by td_main on 2023/11/3.
//

#pragma once
#include <doodle_core/core/core_sql.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/hana.hpp>

#include <core/core_sql.h>
#include <entt/entt.hpp>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
namespace doodle::snapshot {

class sqlite_snapshot {
  struct save_snapshot_t {
    entt::snapshot snapshot_;
    entt::meta_func begin_save_func;
    entt::meta_func save_func;
    entt::entity entity_{};
    entt::meta_any any_data_;
    bool is_entity_{false};
    conn_ptr conn_ptr_;
    explicit save_snapshot_t(entt::registry& in_registry, conn_ptr in_conn)
        : snapshot_{in_registry}, conn_ptr_{std::move(in_conn)} {}

    template <typename Component>
    auto save() {
      set_save_func<Component>();
      if (!begin_save_func) return *this;
      is_entity_ = std::is_same_v<Component, entt::entity>;
      any_data_  = begin_save_func.invoke({}, conn_ptr_);
      snapshot_.get<Component>(*this);
      return *this;
    }
    template <typename Component, typename It>
      requires(!std::is_same_v<Component, entt::entity>)
    auto save(It first, It last) {
      set_save_func<Component>();
      if (!begin_save_func) return *this;
      is_entity_ = std::is_same_v<Component, entt::entity>;
      any_data_  = begin_save_func.invoke({}, conn_ptr_);
      snapshot_.get<Component>(*this, first, last);
      return *this;
    }

   private:
    template <typename Component>
    void set_save_func() {
      auto l_mate     = entt::resolve<Component>();
      begin_save_func = l_mate.func("begin_save"_hs);
      save_func       = l_mate.func("save"_hs);
    }

    inline void operator()(std::underlying_type_t<entt::entity> in_underlying_type) {}
    // 然后是实体和对应组件的循环
    inline void operator()(entt::entity in_entity) {
      entity_ = in_entity;
      if (is_entity_) save_func.invoke(in_entity, any_data_, entt::forward_as_meta(conn_ptr_));
    }

    // 组件的加载和保存
    template <typename T>
    void operator()(const T& in_t) {
      save_func.invoke(in_t, entity_, any_data_, entt::forward_as_meta(conn_ptr_));
    }
  };

  struct load_snapshot_t {
    entt::snapshot_loader loader_;
    entt::meta_func begin_load_func;
    entt::meta_func load_func;
    entt::meta_func get_size_func;
    entt::meta_any any_data_;
    conn_ptr conn_ptr_;
    explicit load_snapshot_t(entt::registry& in_registry, conn_ptr in_conn)
        : loader_{in_registry}, conn_ptr_{std::move(in_conn)} {}

    template <typename Registry>
    friend class entt::basic_snapshot_loader;

    template <typename Component>
    auto load() {
      set_load_func<Component>();
      if (!begin_load_func) return *this;
      any_data_ = begin_load_func.invoke({});
      loader_.get<Component>(*this);
      return *this;
    }

    template <typename Component>
    void set_load_func() {
      auto l_mate     = entt::resolve<Component>();
      get_size_func   = l_mate.func("get_size"_hs);
      begin_load_func = l_mate.func("begin_load"_hs);
      load_func       = l_mate.func("load"_hs);
    }
    // load
    inline void operator()(std::underlying_type_t<entt::entity>& in_underlying_type) {
      in_underlying_type = get_size_func.invoke({}).cast<std::underlying_type_t<entt::entity>>();
    }
    inline void operator()(entt::entity& in_entity) {
      load_func.invoke(in_entity, any_data_, entt::forward_as_meta(conn_ptr_));
    }
    template <typename T>
    void operator()(T& in_t) {
      load_func.invoke(in_t, any_data_, entt::forward_as_meta(conn_ptr_));
    }
  };

  FSys::path data_path_{};
  entt::registry& registry_;

  void init_base_meta();

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
  }

  template <typename... Component>
  void load() {
    database_info l_info{};
    l_info.path_ = data_path_;
    load_snapshot_t l_load{registry_, l_info.get_connection_const()};
    auto l_tx = sqlpp::start_transaction(*l_load.conn_ptr_);
    (l_load.template load<Component>(), ...);
  }
};

}  // namespace doodle::snapshot