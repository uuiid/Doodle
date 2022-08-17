//
// Created by TD on 2021/10/30.
//

#pragma once
#include <entt/entt.hpp>
// #include <boost/range.hpp>
#include <fmt/format.h>

#include <iterator>
#include <doodle_core/lib_warp/enum_template_tool.h>
namespace doodle {
using namespace entt::literals;
namespace entt_tool {
namespace detail {
/**
 * @copydoc save_comm(entt::handle &in_handle, Archive &in_archive)
 */
template <typename Component, typename Archive>
bool _save_(const entt::handle &in_handle, std::size_t in_size, Archive &in_archive) {
  auto &&k_comm      = in_handle.template get<Component>();
  in_archive["data"] = k_comm;
  return true;
}

/**
 * @copydoc load_comm
 */
template <typename Component, typename Archive>
bool _load_(entt::handle &in_handle, Archive &in_archive) {
  Component l_component{};
  if (in_archive.contains(typeid(Component).name()))
    in_archive.at(typeid(Component).name()).get_to(l_component);
  else if (in_archive.contains("data"))
    in_archive.at("data").get_to(l_component);
  in_handle.template emplace_or_replace<Component>(std::move(l_component));
  return true;
}

/**
 * @brief json保存函数
 *
 * @tparam Component 多个实体
 * @tparam Archive  存档类型
 * @tparam Index
 * @param in_handle 传入的句柄
 * @param in_archive 存档
 */
template <typename... Component, typename Archive, std::size_t... Index>
void _save_comm_(const entt::handle &in_handle, Archive &in_archive, std::index_sequence<Index...>) {
  std::array<std::size_t, sizeof...(Index)> size{};
  ((in_handle.template any_of<Component>() ? ++(size[Index]) : 0u), ...);
  ((size[Index] ? _save_<Component>(in_handle, size[Index], in_archive[std::to_string(entt::type_id<Component>().hash())]) : 0u), ...);
}
/**
 * @brief json加载函数
 *
 * @tparam Component  多个实体
 * @tparam Archive 存档类型
 * @tparam Index
 * @param in_handle 传入的句柄
 * @param in_archive 存档
 */
template <typename... Component, typename Archive, std::size_t... Index>
void _load_comm_(entt::handle &in_handle, Archive &in_archive, std::index_sequence<Index...>) {
  ((in_archive.contains(typeid(Component).name())
        ? _load_<Component>(in_handle, in_archive.at(typeid(Component).name()))
        : 0U),
   ...);
  ((in_archive.contains(std::to_string(entt::type_id<Component>().hash()))
        ? _load_<Component>(in_handle, in_archive.at(std::to_string(entt::type_id<Component>().hash())))
        : 0U),
   ...);
}

}  // namespace detail

/**
 * @brief json保存函数
 *
 * @tparam Component 多个组件
 * @tparam Archive 存档类型
 * @param in_handle 句柄
 * @param in_archive 存档
 */
template <typename... Component, typename Archive>
void save_comm(const entt::handle &in_handle, Archive &in_archive) {
  detail::_save_comm_<Component...>(in_handle, in_archive, std::index_sequence_for<Component...>{});
}

/**
 * @brief json加载函数
 *
 * @tparam Component 多个组件
 * @tparam Archive 存档类型
 * @param in_handle 句柄
 * @param in_archive 存档
 */
template <typename... Component, typename Archive>
void load_comm(entt::handle &in_handle, Archive &in_archive) {
  detail::_load_comm_<Component...>(in_handle, in_archive, std::index_sequence_for<Component...>{});
}

}  // namespace entt_tool
}  // namespace doodle

namespace fmt {
/**
 * @brief 集数格式化程序
 *
 * @tparam
 */
template <>
struct formatter<::entt::entity>
    : formatter<typename ::entt::entt_traits<::entt::entity>::entity_type> {
  using base_type = formatter<typename ::entt::entt_traits<::entt::entity>::entity_type>;

  template <typename FormatContext>
  auto format(const ::entt::entity &in_, FormatContext &ctx) const -> decltype(ctx.out()) {
    return base_type::format(
        ::entt::to_integral(in_),
        ctx);
  }
};

template <typename Entity, typename... Type>
struct formatter<::entt::basic_handle<Entity, Type...>>
    : formatter<Entity> {
  using base_type        = formatter<Entity>;
  using entt_handle_type = ::entt::basic_handle<Entity, Type...>;

  template <typename FormatContext>
  auto format(const entt_handle_type &in_, FormatContext &ctx) const -> decltype(ctx.out()) {
    return base_type::format(
        in_.entity(),
        ctx);
  }
};

}  // namespace fmt
