//
// Created by TD on 2021/5/26.
//

#pragma once
#include <doodle_app/doodle_app_fwd.h>

namespace doodle::core {
/**
 * @brief 标识符生成器， 线程安全
 *
 */
class DOODLE_CORE_API identifier {
  identifier();

  virtual ~identifier();
  std::atomic_uint64_t id_;

 public:
  static identifier& get();
  std::uint64_t id();
};
}  // namespace doodle
