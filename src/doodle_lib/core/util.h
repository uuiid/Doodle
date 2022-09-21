//
// Created by TD on 2021/5/26.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

namespace core {
/**
 * @brief 标识符生成器， 线程安全
 *
 */
class DOODLELIB_API identifier {
  identifier();

  ~identifier();
  std::atomic_uint64_t id_;

 public:
  static identifier& get();
  std::uint64_t id();
};
}  // namespace core

}  // namespace doodle
