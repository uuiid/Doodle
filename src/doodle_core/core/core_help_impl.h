//
// Created by TD on 2022/2/27.
//

#pragma once

#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/core/template_util.h>

#include <entt/entt.hpp>

namespace entt {
/**
 * @brief entt 中自定义hash检查
 * @tparam Type doodle metadata 中的类
 */
template <typename Type>
struct [[maybe_unused]] entt::type_hash<Type, std::void_t<decltype(Type::class_hash())>> {
  static entt::id_type value() noexcept { return Type::class_hash(); }
};
}  // namespace entt

namespace doodle {
using registry_ptr = std::shared_ptr<entt::registry>;
DOODLE_CORE_API registry_ptr &g_reg();

template <typename Component, typename Reg_Ptr = registry_ptr>
entt::handle make_handle(Component instance, Reg_Ptr &in_reg_ptr = g_reg()) {
  return entt::handle{*(in_reg_ptr), entt::to_entity(*(in_reg_ptr), *instance)};
}
// template <class Component, typename Reg_Ptr, std::enable_if_t<!std::is_pointer_v<Component>> * = 0>
// entt::handle make_handle(const Component &instance, Reg_Ptr &in_reg_ptr = g_reg()) {
//   return entt::handle{*(in_reg_ptr), entt::to_entity(in_reg_ptr, instance)};
// }

// inline entt::handle make_handle(registry_ptr &in_registry_ptr) {
//   return entt::handle{*(in_registry_ptr), in_registry_ptr->create()};
// }

template <typename Handle_, std::enable_if_t<std::is_same_v<entt::handle, Handle_>, bool> = true>
void destroy_handle(Handle_ &in_handle) {
  if (in_handle) in_handle.destroy();
}

template <typename Container_, std::enable_if_t<details::is_handle_container<Container_>::value, bool> = true>
void destroy_handle(Container_ &in_handles) {
  for (auto &&i : in_handles) destroy_handle(i);
  in_handles.clear();
}

namespace details {
/**
 * @brief 不可复制类
 *
 */
class no_copy {
 public:
  no_copy()                           = default;
  no_copy(const no_copy &)            = delete;
  no_copy &operator=(const no_copy &) = delete;

  no_copy(no_copy &&)                 = default;
  no_copy &operator=(no_copy &&)      = default;
};
}  // namespace details

}  // namespace doodle
