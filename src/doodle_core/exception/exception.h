#pragma once

#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/doodle_core_pch.h>
#include <boost/exception/exception.hpp>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <spdlog/common.h>

namespace doodle {
class DOODLE_CORE_EXPORT doodle_error : public std::runtime_error {
 public:
  explicit doodle_error(const std::string& message) : std::runtime_error(message){};
};
// iterators
class DOODLE_CORE_EXPORT error_iterator : public std::runtime_error {
 public:
  explicit error_iterator(const std::string& message) : std::runtime_error(message){};
};
// 空指针错误
class DOODLE_CORE_EXPORT nullptr_error : public doodle_error {
 public:
  explicit nullptr_error(const std::string& err) : doodle_error(err){};
};

// 序列化错误
class DOODLE_CORE_EXPORT serialization_error : public doodle_error {
 public:
  explicit serialization_error(const std::string& err) : doodle_error(err){};
};

class DOODLE_CORE_EXPORT component_error : public doodle_error {
 public:
  explicit component_error(const std::string& err) : doodle_error(err){};
};

class DOODLE_CORE_EXPORT file_error : public doodle_error {
 public:
  explicit file_error(const std::string& message)
      : doodle_error(message){};
};

template <typename exception_type>
[[noreturn]] void throw_exception(exception_type&& in_exception_type, ::boost::source_location const& loc = BOOST_CURRENT_LOCATION) {
  boost::throw_exception(std::forward<exception_type>(in_exception_type), loc);
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
  auto format(const ::doodle::doodle_error& in_, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<string_view>::format(
        in_.what(),
        ctx);
  }
};
}  // namespace fmt
