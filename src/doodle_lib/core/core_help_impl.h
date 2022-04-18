//
// Created by TD on 2022/2/27.
//

#pragma once

#include <entt/entt.hpp>
namespace doodle {
/**
 * @brief entt 中自定义hash检查
 * @tparam Type doodle metadata 中的类
 */
template <typename Type>
struct [[maybe_unused]] entt::type_hash<
    Type,
    std::void_t<decltype(Type::class_hash())>> {
  static entt::id_type value() ENTT_NOEXCEPT {
    return Type::class_hash();
  }
};
using registry_ptr = std::shared_ptr<entt::registry>;
registry_ptr &g_reg();

template <class Component,
          std::enable_if_t<!std::is_same_v<entt::entity, Component>, bool> = true>
entt::handle make_handle(const Component &instance) {
  return entt::handle{*(g_reg()), entt::to_entity(*(g_reg()), instance)};
};
template <class Component,
          std::enable_if_t<std::is_same_v<entt::entity, Component>, bool> = true>
entt::handle make_handle(const Component &instance) {
  return entt::handle{*(g_reg()), instance};
};

inline entt::handle make_handle() {
  return entt::handle{*(g_reg()), g_reg()->create()};
};

class DOODLELIB_API null_fun_t {
 public:
  null_fun_t() = default;
  template <class in_class>
  inline void operator()(in_class &in){};
};

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
