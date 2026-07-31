//
// Created by TD on 26-7-31.
//
#pragma once

#include <nlohmann/json.hpp>

namespace doodle::ai {

/// @brief 运动 mask 模式
enum class motion_mask_mode {
  none,    ///< 无运动 mask
  concat,  ///< motion_mask 与 x 拼接作为额外通道输入
};

NLOHMANN_JSON_SERIALIZE_ENUM(motion_mask_mode, {
  {motion_mask_mode::none, "none"},
  {motion_mask_mode::concat, "concat"},
})

}  // namespace doodle::ai