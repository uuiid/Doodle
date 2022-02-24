//
// Created by TD on 2021/5/26.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
template <class T_, class Tag>
class temp_warp : public T_ {
 public:
  //  temp_warp() = default;

  template <class... Arg,
            std::enable_if_t<std::is_trivially_constructible<T_, Arg...>::value, bool> = true>
  explicit temp_warp(Arg... args) : T_(std::forward<Arg...>(args...)){};

  using T_::T_;
  using T_ ::operator=;
};

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
