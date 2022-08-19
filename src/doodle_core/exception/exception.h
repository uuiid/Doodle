#pragma once

#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/doodle_core_pch.h>
#include <boost/exception/exception.hpp>
#include <boost/throw_exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/system.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <spdlog/common.h>

namespace doodle {

namespace bsys = boost::system;

enum class error_enum : std::int32_t {
  success = 0,
  sqlite3_save_error,
  file_copy_error,
  component_missing_error,
  file_not_exists
};

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

class DOODLE_CORE_EXPORT doodle_category : public bsys::error_category {
 public:
  const char* name() const noexcept;

  std::string message(int ev) const;
  char const* message(int ev, char* buffer, std::size_t len) const noexcept;

  bool failed(int ev) const noexcept;

  bsys::error_condition default_error_condition(int ev) const noexcept;

  static const bsys::error_category& get();
};

bsys::error_code make_error_code(error_enum e, ::boost::source_location const* in_loc);
bsys::error_code make_error_code(error_enum e);

template <typename exception_type>
[[noreturn]] void throw_exception(exception_type&& in_exception_type, ::boost::source_location const& in_loc = BOOST_CURRENT_LOCATION) {
  boost::throw_exception(std::forward<exception_type>(in_exception_type), in_loc);
}
#define DOODLE_CHICK(condition, ...) \
  if (!(condition)) {                \
    throw_exception(__VA_ARGS__);    \
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
        boost::diagnostic_information(in_),
        ctx);
  }
};
}  // namespace fmt

namespace boost::system {
template <>
struct is_error_code_enum<::doodle::error_enum> : std::true_type {};
}  // namespace boost::system
namespace std {
template <>
struct is_error_code_enum<::doodle::error_enum> : std::true_type {};
}  // namespace std
