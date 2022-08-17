#pragma once

#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/doodle_core_pch.h>
#include <boost/exception/exception.hpp>
#include <boost/throw_exception.hpp>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <spdlog/common.h>

namespace doodle {
class DOODLE_CORE_EXPORT doodle_error : public std::runtime_error {
 public:
  explicit doodle_error(const std::string& message) : std::runtime_error(message){};
  template <typename... Args>
  explicit doodle_error(const std::string& fmt_str, Args&&... in_args)
      : std::runtime_error(fmt::format(fmt_str, std::forward<Args>(in_args)...)){};
};
// iterators
class DOODLE_CORE_EXPORT error_iterator : public std::runtime_error {
 public:
  explicit error_iterator(const std::string& message) : std::runtime_error(message){};
};

class DOODLE_CORE_EXPORT component_error : public doodle_error {
 public:
  explicit component_error(const std::string& err) : doodle_error(err){};
};

template <typename exception_type>
[[noreturn]] void throw_exception(exception_type&& in_exception_type, ::boost::source_location const& in_loc = BOOST_CURRENT_LOCATION) {
  boost::throw_exception(std::forward<exception_type>(in_exception_type), in_loc);
}
#define DOODLE_CHICK(condition, ...)                      \
  if (!(condition)) {                                     \
    throw_exception(__VA_ARGS__, BOOST_CURRENT_LOCATION); \
  }

}  // namespace doodle

#include <fmt/format.h>
namespace fmt {
/**
 * @brief 格式化库异常
 *
 * @tparam
 */
template <>
struct formatter<::doodle::doodle_error> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const ::doodle::doodle_error& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<string_view>::format(
        in_.what(),
        ctx);
  }
};
}  // namespace fmt
