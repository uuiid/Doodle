//
// Created by TD on 2021/9/17.
//

#pragma once

#include <fmt/format.h>
#include <boost/hana.hpp>
#include <memory>
namespace doodle {
namespace details {

}  // namespace details

template <class ClassIn, class... Args>
std::shared_ptr<ClassIn> new_object(Args &&...in_args) {
  // post_constructor
  constexpr auto has_make_this =
      boost::hana::is_valid(
          [](auto &&obj)
              -> decltype(obj->post_constructor()) {}
      );
  auto ptr              = std::make_shared<ClassIn>(std::forward<Args>(in_args)...);
  using has_make_this_v = decltype(has_make_this(ptr));
  if constexpr (has_make_this_v{}) {
    ptr->post_constructor();
  }
  return ptr;
}
}  // namespace doodle

namespace fmt {
/**
 * @brief 格式化::std::filesystem::path
 *
 * @tparam  ::std::filesystem::path
 */
template <>
struct formatter<::std::filesystem::path> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const ::std::filesystem::path &in_, FormatContext &ctx) const -> decltype(ctx.out()) {
    return formatter<fmt::string_view>::format(
        in_.generic_string(),
        ctx
    );
  }
};
}  // namespace fmt
