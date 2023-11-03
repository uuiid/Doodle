//
// Created by td_main on 2023/11/3.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/hana.hpp>

#include <entt/entt.hpp>
namespace doodle::snapshot {

template <typename... Component>
class sqlite_snapshot {
  std::tuple<std::deque<std::pair<entt::entity, Component>>...> tuple_{};
  std::underlying_type_t<entt::entity> current_component_size_{};
  entt::entity current_entity_{};
  static constexpr auto hana_tup_ = boost::hana::make_tuple(boost::hana::type_c<Component>...);

  FSys::path data_path_{};

 public:
  explicit sqlite_snapshot(FSys::path in_data_path)
      : data_path_(in_data_path.empty() ? FSys::path{":memory:"} : std::move(in_data_path)) {}

  virtual ~sqlite_snapshot() = default;

  template <typename It, typename... Component_1>
  void save(
      const entt::registry& in_registry,
      const boost::hana::map<boost::hana::pair<Component_1, std::pair<It, It>>...>& in_map_iter
  );

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
    l_vector.pop_front();

    if (l_vector.empty()) {
      constexpr auto l_index = *boost::hana::index_if(hana_tup_, boost::hana::is_a<T>);
      if constexpr (l_index + 1 < sizeof...(Component)) {
        current_component_size_ = std::get<l_index + 1>(tuple_).size();
        current_entity_         = std::get<l_index + 1>(tuple_).front().first;
      }
    } else {
      current_entity_ = l_vector.front().first;
    }
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
template <typename... Component>
void sqlite_snapshot<Component...>::operator()(std::underlying_type_t<entt::entity>& in_underlying_type) {
  in_underlying_type = current_component_size_;
}
template <typename... Component>
void sqlite_snapshot<Component...>::operator()(entt::entity& in_entity) {
  in_entity = current_entity_;
}

template <typename... Component>
template <typename It, typename... Component_1>
void sqlite_snapshot<Component...>::save(
    const entt::registry& in_registry,
    const boost::hana::map<boost::hana::pair<Component_1, std::pair<It, It>>...>& in_map_iter
) {}

}  // namespace doodle::snapshot