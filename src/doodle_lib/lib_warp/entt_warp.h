//
// Created by TD on 2021/10/30.
//

#pragma once

#include <DoodleConfig.h>

#include <entt/entt.hpp>
namespace doodle::entt_tool {
namespace detail {
template <class Component, class Archive>
void _save_(entt::handle &in_handle, std::size_t in_size, Archive &in_archive) {
  in_archive[typeid(in_size).name()] = in_size;
  while (in_size--) {
    in_archive[typeid(Component).name()] = in_handle.template get<Component>();
  }
}

template <class Component, class Archive>
void _load_(entt::handle &in_handle, Archive &in_archive) {
  std::size_t l_size{};
  l_size = in_archive[typeid(l_size).name()].template get<std::size_t>();

  while (l_size--) {
    auto &l_component = in_archive[typeid(Component).name()].template get<Component>();
    in_handle.template emplace_or_replace<Component>(std::move(l_component));
  }
}

template <class... Component, class Archive, std::size_t... Index>
void _save_comm_(entt::handle &in_handle, Archive &in_archive, std::index_sequence<Index...>) {
  std::set<string> k_set{(boost::serialization::guid<Component>(), ...)};
  in_archive[typeid(k_set).name()] = k_set;

  std::array<std::size_t, sizeof...(Index)> size{};
  ((in_handle.template any_of<Component>() ? ++(size[Index]) : 0u), ...);
  (_save_<Component>(in_handle, size[Index], in_archive), ...);
}

template <class... Component, class Archive, std::size_t... Index>
void _load_comm_(entt::handle &in_handle, Archive &in_archive, std::index_sequence<Index...>) {
  auto &k_set = in_archive[typeid(std::set<string>).name()].template get<std::set<string>>();

  (((k_set.template count(boost::serialization::guid<Component>()) > 1)
        ? _load_<Component>(in_handle, in_archive)
        : 0u),
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

}  // namespace doodle::entt_tool
