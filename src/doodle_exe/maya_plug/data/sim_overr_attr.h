//
// Created by TD on 2022/1/7.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::maya_plug {

class sim_overr_attr {
 private:
 public:
  sim_overr_attr();
  /**
   * @brief 调整参数
   */
  bool use_overr_attr;
  /**
   * @brief 精密解算
   */
  bool simple_subsampling;
  /**
   * @brief 帧样本 1.f <-> 50.f
   */
  std::float_t frame_samples;
  /**
   * @brief 时间尺度 0.1f <-> 10.f
   */
  std::float_t time_scale;
  /**
   * @brief 长度尺度 0.1f <-> 10.f
   */
  std::float_t length_scale;
  /**
   * @brief 尖锐碰撞
   */
  bool sharp_feature;
};

}  // namespace doodle::maya_plug
