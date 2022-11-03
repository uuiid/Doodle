//
// Created by TD on 2021/5/26.
//

#pragma once
#include <doodle_app/doodle_app_fwd.h>

#include <atomic>

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

/**
 * @brief 这是一个bool锁定类
 *
 */
class bool_mutex {
 public:
  std::atomic_bool data{};

  inline void lock() { data = true; }
  inline void unlock() { data = false; }
  [[nodiscard]] inline explicit operator bool() const { return data; }
};

}  // namespace doodle::core
