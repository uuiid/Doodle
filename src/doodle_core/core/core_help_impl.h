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
struct [[maybe_unused]] entt::type_hash<
    Type,
    std::void_t<decltype(Type::class_hash())>> {
  static entt::id_type value() noexcept { return Type::class_hash(); }
};
}  // namespace entt

namespace doodle {
using registry_ptr = std::shared_ptr<entt::registry>;
DOODLE_CORE_API registry_ptr &g_reg();

template <class Component, std::enable_if_t<!std::is_same_v<entt::entity, Component>, bool> = true>
entt::handle make_handle(const Component &instance) {
  return entt::handle{*(g_reg()), entt::to_entity(*(g_reg()), instance)};
};
template <class Component, std::enable_if_t<std::is_same_v<entt::entity, Component>, bool> = true>
entt::handle make_handle(const Component &instance) {
  return entt::handle{*(g_reg()), instance};
};

inline entt::handle make_handle() {
  return entt::handle{*(g_reg()), g_reg()->create()};
}

// template <class Component, std::enable_if_t<!std::is_same_v<entt::entity, Component>, bool> = true>
// entt::handle make_gui_handle(const Component &instance) {
//   return entt::handle{*(g_gui_reg()), entt::to_entity(*(g_gui_reg()), instance)};
// };
// template <class Component, std::enable_if_t<std::is_same_v<entt::entity, Component>, bool> = true>
// entt::handle make_gui_handle(const Component &instance) {
//   return entt::handle{*(g_gui_reg()), instance};
// };
//
// inline entt::handle make_gui_handle() {
//   return entt::handle{*(g_gui_reg()), g_gui_reg()->create()};
// }

template <typename Handle_, std::enable_if_t<std::is_same_v<entt::handle, Handle_>, bool> = true>
void destroy_handle(Handle_ &in_handle) {
  if (in_handle)
    in_handle.destroy();
}

template <typename Container_, std::enable_if_t<details::is_handle_container<Container_>::value, bool> = true>
void destroy_handle(Container_ &in_handles) {
  for (auto &&i : in_handles)
    destroy_handle(i);
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
