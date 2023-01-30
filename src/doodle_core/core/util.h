//
// Created by TD on 2021/5/26.
//

#pragma once
#include <doodle_app/doodle_app_fwd.h>

#include <atomic>

namespace doodle::details {
/**
 * @brief 标识符生成器， 线程安全
 *
 */
class DOODLE_CORE_API identifier {
  mutable std::atomic_uint64_t id_;

 public:
  identifier();
  ~identifier();

  std::uint64_t id() const;

  inline explicit operator std::uint64_t() const { return id(); }
};

}  // namespace doodle::details

namespace fmt {

template <>
struct formatter<::doodle::details::identifier> : formatter<std::uint64_t> {
  template <typename FormatContext>
  auto format(const ::doodle::details::identifier &in_, FormatContext &ctx) const -> decltype(ctx.out()) {
    return formatter<std::uint64_t>::format(in_.id(), ctx);
  }
};

}  // namespace fmt