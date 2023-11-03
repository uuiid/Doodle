//
// Created by td_main on 2023/7/12.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include "entt/core/type_traits.hpp"
#include <filesystem>

namespace doodle {
namespace detail {
template <typename Id_Type>
class one_file_base {
 public:
  FSys::path path_{};
  constexpr static auto id = Id_Type::value;
};

}  // namespace detail
}  // namespace doodle
