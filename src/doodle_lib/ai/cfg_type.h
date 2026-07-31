//
// Created by TD on 26-7-31.
//
#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace doodle::ai {

/// @brief 无分类器引导（CFG）类型
enum class cfg_type {
  default_,   ///< 使用模型默认值
  nocfg,      ///< 无引导，直接调用 model.forward
  regular,    ///< out_uncond + w * (out_cond - out_uncond)
  separated   ///< out_uncond + w_text*(out_text - out_uncond) + w_constraint*(out_constraint - out_uncond)
};

NLOHMANN_JSON_SERIALIZE_ENUM(cfg_type, {
  {cfg_type::default_, "default"},
  {cfg_type::nocfg, "nocfg"},
  {cfg_type::regular, "regular"},
  {cfg_type::separated, "separated"},
})

}  // namespace doodle::ai