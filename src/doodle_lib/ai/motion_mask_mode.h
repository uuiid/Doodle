//
// Created by TD on 26-7-31.
//
#pragma once

#include <doodle_core/lib_warp/json_warp.h>

#include <nlohmann/json.hpp>

namespace doodle::ai {

/// @brief 运动 mask 模式
enum class motion_mask_mode {
  none,    ///< 无运动 mask
  concat,  ///< motion_mask 与 x 拼接作为额外通道输入
};

DOODLE_JSON_SERIALIZE_ENUM_STRICT(motion_mask_mode, {
  {motion_mask_mode::none, "none"},
  {motion_mask_mode::concat, "concat"},
})

}  // namespace doodle::ai