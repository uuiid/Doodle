//
// Created by td_main on 2023/7/12.
//

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
using maya_file_id     = entt::tag<"maya_file"_hs>;
using ue_file_id       = entt::tag<"ue_file"_hs>;
using maya_rig_file_id = entt::tag<"maya_rig_file"_hs>;
}  // namespace detail

using maya_file     = detail::one_file_base<detail::maya_file_id>;
using ue_file       = detail::one_file_base<detail::ue_file_id>;
using maya_rig_file = detail::one_file_base<detail::maya_rig_file_id>;

}  // namespace doodle
