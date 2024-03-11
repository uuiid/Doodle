//
// Created by td_main on 2023/11/3.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/hana.hpp>

#include <entt/entt.hpp>
namespace doodle::snapshot {

class sqlite_snapshot {
  FSys::path data_path_{};
  entt::registry& registry_;
  std::unique_ptr<entt::snapshot> snapshot_{};
  std::unique_ptr<entt::snapshot_loader> loader_{};
  template <typename Registry>
  friend class entt::basic_snapshot;
  template <typename Registry>
  friend class basic_snapshot_loader;

 public:
  explicit sqlite_snapshot(FSys::path in_data_path, entt::registry& in_registry)
      : data_path_(in_data_path.empty() ? FSys::path{":memory:"} : std::move(in_data_path)), registry_{in_registry} {}

  virtual ~sqlite_snapshot() = default;

  template <typename Component>
    requires std::is_same_v<Component, entt::entity>
  auto load() {
    if (!loader_) {
      loader_ = std::make_unique<entt::snapshot_loader>(registry_);
    }
    loader_->get<Component>(*this);
    return *this;
  }
  template <typename Component>
    requires(!std::is_same_v<Component, entt::entity>)
  auto load() {
    if (!loader_) {
      loader_ = std::make_unique<entt::snapshot_loader>(registry_);
    }
    loader_->get<Component>(*this);
    return *this;
  }
  template <typename Component>
    requires std::is_same_v<Component, entt::entity>
  auto save() {
    if (!snapshot_) {
      snapshot_ = std::make_unique<entt::snapshot>(registry_);
    }

    snapshot_->get<Component>(*this);
    return *this;
  }
  template <typename Component>
    requires(!std::is_same_v<Component, entt::entity>)
  auto save() {
    if (!snapshot_) {
      snapshot_ = std::make_unique<entt::snapshot>(registry_);
    }
    snapshot_->get<Component>(*this);
    return *this;
  }

 private:
  // 先是大小
  void operator()(std::underlying_type_t<entt::entity> in_underlying_type);
  // 然后是实体和对应组件的循环
  void operator()(entt::entity in_entity);

  void operator()(std::underlying_type_t<entt::entity>& in_underlying_type);
  void operator()(entt::entity& in_entity);

  template <typename Component>
  void save(FSys::path in_data_path, const std::vector<std::int64_t>& in_delete_id);

  // 组件的加载和保存
  template <typename T>
  void operator()(const T& in_t) {}
  template <typename T>
  void operator()(T& in_t) {}
};

}  // namespace doodle::snapshot