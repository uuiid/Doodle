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

}  // namespace doodle
