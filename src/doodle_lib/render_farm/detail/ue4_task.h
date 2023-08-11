//
// Created by td_main on 2023/8/11.
//

#pragma once
namespace doodle {
namespace render_farm {
namespace detail {

class ue4_task {};

}  // namespace detail
using ue4_task_ptr = std::shared_ptr<detail::ue4_task>;
}  // namespace render_farm
}  // namespace doodle
