//
// Created by td_main on 2023/11/3.
//

#pragma once
#include <boost/hana.hpp>

#include <entt/entt.hpp>
namespace doodle::snapshot {

template <typename... Component>
class sqlite_snapshot {
  std::tuple<std::vector<std::pair<entt::entity, Component>>...> tuple_{};
  std::underlying_type_t<entt::entity> current_component_size_{};
  entt::entity current_entity_{};

  boost::hana::map<boost::hana::pair<Component, std::underlying_type_t<entt::entity>>...> size_;

 public:
  sqlite_snapshot()          = default;
  virtual ~sqlite_snapshot() = default;
  // 先是大小
  void operator()(std::underlying_type_t<entt::entity> in_underlying_type);
  // 然后是实体和对应组件的循环
  void operator()(entt::entity in_entity);

  void operator()(std::underlying_type_t<entt::entity>& in_underlying_type);
  void operator()(entt::entity& in_entity);

  // 组件的加载和保存
  template <typename T>
  void operator()(const T& in_t) {
    if (current_component_size_ != 0) {
      std::get<std::vector<T>>(tuple_).reserve(current_component_size_);
      current_component_size_ = 0;
    }
    std::get<std::vector<T>>(tuple_).emplace_back(current_entity_, in_t);
  }
  template <typename T>
  void operator()(T& in_t) {
    auto& l_vector = std::get<std::vector<T>>(tuple_);
    in_t           = std::move(l_vector.front().second);
  }
};

template <typename... Component>
void sqlite_snapshot<Component...>::operator()(std::underlying_type_t<entt::entity> in_underlying_type) {
  current_component_size_ = in_underlying_type;
}
template <typename... Component>
void sqlite_snapshot<Component...>::operator()(entt::entity in_entity) {
  current_entity_ = in_entity;
}

}  // namespace doodle::snapshot