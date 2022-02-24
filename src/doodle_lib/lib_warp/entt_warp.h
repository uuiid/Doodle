//
// Created by TD on 2021/10/30.
//

#pragma once

#include <DoodleConfig.h>

#include <entt/entt.hpp>
// #include <boost/range.hpp>
#include <fmt/format.h>

#include <iterator>

namespace doodle {
namespace entt_tool {
namespace detail {
/**
 * @copydoc save_comm(entt::handle &in_handle, Archive &in_archive)
 */
template <typename Component, typename Archive>
bool _save_(entt::handle &in_handle, std::size_t in_size, Archive &in_archive) {
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
void _save_comm_(entt::handle &in_handle, Archive &in_archive, std::index_sequence<Index...>) {
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
void save_comm(entt::handle &in_handle, Archive &in_archive) {
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

namespace boost {
/**
 * @brief boost range 库范围适配器
 *
 * @tparam E 实体类型
 * @tparam Get 获取类
 * @tparam Exclude 排除类
 */
template <typename E, typename Get, typename Exclude>
struct range_mutable_iterator<entt::basic_view<E, Get, Exclude>> {
  using entt_view = entt::basic_view<E, Get, Exclude>;
  using type      = typename entt_view::iterator;
};
/**
 * @brief boost range 库范围适配器(常量迭代器)
 *
 * @tparam E 实体类型
 * @tparam Get 获取类
 * @tparam Exclude 排除类
 */
template <typename E, typename Get, typename Exclude>
struct range_const_iterator<entt::basic_view<E, Get, Exclude>> {
  using entt_view = entt::basic_view<E, Get, Exclude>;
  using type      = typename entt_view::iterator;
};
}  // namespace boost

namespace entt {
/**
 * @brief boost rang 范围适配器 获取开始迭代器
 *
 * @tparam E 实体类型
 * @tparam Get 获取类
 * @tparam Exclude 排除类
 * @param x 传入的entt视图
 * @return auto 开始迭代器
 */
template <typename E, typename Get, typename Exclude>
inline auto range_begin(entt::basic_view<E, Get, Exclude> &x) {
  return x.begin();
}
/**
 * @brief boost rang 范围适配器 获取开始迭代器(常量版本)
 *
 * @tparam E 实体类型
 * @tparam Get 获取类
 * @tparam Exclude 排除类
 * @param x 传入的entt视图
 * @return auto 开始迭代器
 */
template <typename E, typename Get, typename Exclude>
inline auto range_begin(const entt::basic_view<E, Get, Exclude> &x) {
  return x.begin();
}
/**
 * @brief boost rang 范围适配器 获取结束迭代器
 *
 * @tparam E 实体类型
 * @tparam Get 获取类
 * @tparam Exclude 排除类
 * @param x 传入的entt视图
 * @return auto 结束迭代器
 */
template <typename E, typename Get, typename Exclude>
inline auto range_end(entt::basic_view<E, Get, Exclude> &x) {
  return x.end();
}
/**
 * @brief boost rang 范围适配器 获取结束迭代器(常量版本)
 *
 * @tparam E 实体类型
 * @tparam Get 获取类
 * @tparam Exclude 排除类
 * @param x 传入的entt视图
 * @return auto 结束迭代器
 */
template <typename E, typename Get, typename Exclude>
inline auto range_end(const entt::basic_view<E, Get, Exclude> &x) {
  return x.end();
}
}  // namespace entt