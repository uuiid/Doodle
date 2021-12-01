//
// Created by TD on 2021/10/30.
//

#pragma once

#include <DoodleConfig.h>

#include <boost/serialization/export.hpp>
#include <boost/serialization/nvp.hpp>
#include <entt/entt.hpp>
namespace doodle::entt_tool {
namespace detail {
template <class Component, class Archive>
void _save_(entt::handle &in_handle, std::size_t in_size, Archive &in_archive) {
  in_archive << boost::make_nvp(typeid(in_size).name(), in_size);
  while (in_size--) {
    in_archive << boost::make_nvp(typeid(Component).name(), in_handle.template get<Component>());
  }
}

template <class Component, class Archive>
void _load_(entt::handle &in_handle, Archive &in_archive) {
  std::size_t l_size{};
  in_archive >> boost::make_nvp(typeid(l_size).name(), l_size);

  while (l_size--) {
    Component l_component{};
    in_archive >> boost::make_nvp(typeid(l_component).name(), l_component);
    in_handle.template emplace_or_replace<Component>(std::move(l_component));
  }
}

template <class... Component, class Archive, std::size_t... Index>
void _save_comm_(entt::handle &in_handle, Archive &in_archive, std::index_sequence<Index...>) {
  std::set<string> k_set{(boost::serialization::guid<Component>(), ...)};
  in_archive << boost::make_nvp(typeid(k_set).name(), k_set);

  std::array<std::size_t, sizeof...(Index)> size{};
  ((in_handle.template any_of<Component>() ? ++(size[Index]) : 0u), ...);
  (_save_<Component>(in_handle, size[Index], in_archive), ...);
}

template <class... Component, class Archive, std::size_t... Index>
void _load_comm_(entt::handle &in_handle, Archive &in_archive, std::index_sequence<Index...>) {
  std::set<string> k_set{};  //(boost::serialization::guid<Component>(), ...)
  in_archive >> boost::make_nvp(typeid(k_set).name(), k_set);

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
