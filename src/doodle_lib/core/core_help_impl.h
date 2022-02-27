//
// Created by TD on 2022/2/27.
//

#pragma once

#include <entt/entt.hpp>
namespace doodle {

template <typename Type>
[[maybe_unused]] struct entt::type_hash<
    Type,
    std::void_t<decltype(Type::class_hash())>> {
  static entt::id_type value() ENTT_NOEXCEPT {
    return Type::class_hash();
  }
};
}  // namespace doodle
