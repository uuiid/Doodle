//
// Created by TD on 2021/10/30.
//

#pragma once

#include <DoodleConfig.h>

#include <entt/entt.hpp>
namespace doodle {
namespace entt_tool {

namespace detail {
template <class Component, class Archive>
void _save_(entt::handle &in_handle, std::size_t in_size, Archive &in_archive) {
  in_archive["size"] = in_size;
  while (in_size--) {
    in_archive[typeid(Component).name()] = in_handle.template get<Component>();
  }
}

template <class Component, class Archive>
void _load_(entt::handle &in_handle, Archive &in_archive) {
  std::size_t l_size{};
  l_size = in_archive.at("size").template get<std::size_t>();

  while (l_size--) {
    Component l_component{};
    in_archive.at(typeid(Component).name()).get_to(l_component);
    in_handle.template emplace_or_replace<Component>(std::move(l_component));
  }
}

template <class... Component, class Archive, std::size_t... Index>
void _save_comm_(entt::handle &in_handle, Archive &in_archive, std::index_sequence<Index...>) {
  std::array<std::size_t, sizeof...(Index)> size{};
  ((in_handle.template any_of<Component>() ? ++(size[Index]) : 0u), ...);
  (_save_<Component>(in_handle, size[Index], in_archive[typeid(Component).name()]), ...);
}

template <class... Component, class Archive, std::size_t... Index>
void _load_comm_(entt::handle &in_handle, Archive &in_archive, std::index_sequence<Index...>) {
  ((in_archive.contains(typeid(Component).name())
        ? _load_<Component>(in_handle, in_archive.at(typeid(Component).name()))
        : 0U),
   ...);
}

}  // namespace detail

template <class... Component, class Archive>
void save_comm(entt::handle &in_handle, Archive &in_archive) {
  detail::_save_comm_<Component...>(in_handle, in_archive, std::index_sequence_for<Component...>{});
}

template <class... Component, class Archive>
void load_comm(entt::handle &in_handle, Archive &in_archive) {
  detail::_load_comm_<Component...>(in_handle, in_archive, std::index_sequence_for<Component...>{});
}

class scoped_function {
 public:
  std::vector<std::function<void()>> fun_list;
  scoped_function() = default;
  inline ~scoped_function();
};
}  // namespace entt_tool
using scoped_function = entt_tool::scoped_function;
}  // namespace doodle
