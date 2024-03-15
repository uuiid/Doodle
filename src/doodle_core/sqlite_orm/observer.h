//
// Created by TD on 2024/3/14.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/sqlite_orm/sqlite_snapshot.h>

namespace doodle::snapshot {

template <typename type_t>
class impl_obs {
  entt::observer obs_update_;
  entt::observer obs_create_;
  std::vector<entt::entity> destroy_ids_{};
  entt::connection conn_{};
  void on_destroy(entt::registry& in_reg, entt::entity in_entt) { destroy_ids_.emplace_back(in_entt); }

 public:
  explicit impl_obs()                  = default;
  impl_obs(const impl_obs&)            = delete;
  impl_obs(impl_obs&&)                 = delete;
  impl_obs& operator=(const impl_obs&) = delete;
  impl_obs& operator=(impl_obs&&)      = delete;
  ~impl_obs()                          = default;

  bool has_data() const { return !obs_update_.empty() || !obs_create_.empty() || !destroy_ids_.empty(); }

  void connect(entt::registry& in_registry_ptr) {
    obs_update_.connect(in_registry_ptr, entt::collector.update<type_t>());
    obs_create_.connect(in_registry_ptr, entt::collector.group<type_t>());
    conn_ = in_registry_ptr.on_destroy<type_t>().connect<&impl_obs::on_destroy>(*this);
  }

  void disconnect() {
    obs_update_.disconnect();
    obs_create_.disconnect();
    if (conn_) conn_.release();
  }

  void clear() {
    obs_update_.clear();
    obs_create_.clear();
    destroy_ids_.clear();
  }
  /**
   *
   * @return 需要保存的数据, 需要删除的数据
   */
  std::tuple<std::vector<entt::entity>, std::vector<std::int64_t>> get_data() const {
    std::vector<entt::entity> l_data{};
    for (const auto& entity : obs_update_) {
      l_data.push_back(entity);
    }
    for (const auto& entity : obs_create_) {
      l_data.push_back(entity);
    }
    l_data |= ranges::actions::unique;
    auto l_destroy_ids = destroy_ids_;
    l_destroy_ids |= ranges::actions::unique;
    auto l_destroy_ids_int = l_destroy_ids |
                             ranges::views::transform([](const auto& in) -> std::int64_t { return enum_to_num(in); }) |
                             ranges::to_vector;

    return {std::move(l_data), l_destroy_ids_int};
  }

  void save(sqlite_snapshot& in_snapshot) {
    auto&& [l_data, l_des] = get_data();
    in_snapshot.save<type_t>(l_data.begin(), l_data.end());
    in_snapshot.destroy(l_des.begin(), l_des.end());
  }
};

template <typename... Arg_Com>
class observer_main {
  using obs_tuple_t = std::tuple<std::shared_ptr<impl_obs<Arg_Com>>...>;
  obs_tuple_t obs_tuple_{};

 public:
  observer_main() : obs_tuple_{std::make_shared<impl_obs<Arg_Com>>()...} {}
  observer_main(const observer_main&)            = default;
  observer_main(observer_main&&)                 = default;
  observer_main& operator=(const observer_main&) = default;
  observer_main& operator=(observer_main&&)      = default;
  ~observer_main()                               = default;

  void connect(entt::registry& in_registry) {
    std::apply([&in_registry](auto&... obs) { (obs->connect(in_registry), ...); }, obs_tuple_);
  }

  void disconnect() {
    std::apply([](auto&... obs) { (obs->disconnect(), ...); }, obs_tuple_);
  }

  void clear() {
    std::apply([](auto&... obs) { (obs->clear(), ...); }, obs_tuple_);
  }

  bool has_data() const {
    return std::apply([](auto&... obs) { return (obs->has_data() || ...); }, obs_tuple_);
  }

  void save(sqlite_snapshot& in_snapshot) {
    in_snapshot.start_tx();
    std::apply([&in_snapshot](auto&... obs) { (obs->save(in_snapshot), ...); }, obs_tuple_);
    in_snapshot.commit();
    clear();
  }
};

}  // namespace doodle::snapshot