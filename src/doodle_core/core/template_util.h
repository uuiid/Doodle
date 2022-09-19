//
// Created by TD on 2022/3/4.
//
#pragma once
#include <type_traits>
#include <entt/fwd.hpp>

namespace doodle::details {

/**
 * @brief 判断是否是智能指针
 *
 * @tparam T 需要判断的类型
 */
template <typename T, typename = void>
struct is_smart_pointer : public std::false_type {};
/**
 * @brief 判断是否是智能指针
 *
 * @tparam T 需要判断的类型
 * @tparam Enable 辅助类
 */

template <typename T>
struct is_smart_pointer<T, std::void_t<decltype(T::element_type)>> : public std::true_type {};

template <typename T, typename = void>
struct is_handle_container : public std::false_type {};

template <typename T>
struct is_handle_container<T, std::enable_if_t<std::is_same_v<entt::handle, typename T::value_type>, void>>
    : public std::true_type {};

/// to boost::less_pointees_t;

}  // namespace doodle::details
