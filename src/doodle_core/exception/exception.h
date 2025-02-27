#pragma once

#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/doodle_core_pch.h>

#include <boost/beast.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception/exception.hpp>
#include <boost/system.hpp>
#include <boost/throw_exception.hpp>

#include <filesystem>
#include <spdlog/common.h>
#include <stdexcept>
#include <string>

namespace doodle {

namespace bsys = boost::system;

namespace maya_enum {
enum class maya_error_t : std::int32_t {
  success             = 0,
  // 未知错误
  unknown_error       = 1,
  // 相机命名错误
  camera_name_error   = 2,
  // 骨骼缩放为 0 错误
  bone_scale_error    = 3,
  // 相机纵横比错误
  camera_aspect_error = 4,
  // 缓存路径不存在
  cache_path_error    = 5,
  // 检查错误
  check_error         = 6

};
[[maybe_unused]] bsys::error_code DOODLE_CORE_API make_error_code(maya_error_t e);

}  // namespace maya_enum

namespace error_enum {
enum error_t : std::int32_t {
  success = 0,
  sqlite3_save_error,
  file_copy_error,
  component_missing_error,
  invalid_handle,
  file_not_exists,
  time_to_tm_error,

  null_string,
  bad_json_string,
  bad_url,
  not_find_work_class,
  // 不允许多个工作
  not_allow_multi_work,
  // 没有找到project
  project_not_exist,
  // 超时
  time_out,
  // 文件存在
  file_exists,
  // 文件夹存在
  file_is_directory,

};
[[maybe_unused]] bsys::error_code DOODLE_CORE_API make_error_code(error_t e);
}  // namespace error_enum
class DOODLE_CORE_API doodle_error : public std::runtime_error {
 public:
  std::int32_t error_code_;
  explicit doodle_error(const std::string& message) : std::runtime_error(message), error_code_(0) {};
  template <typename... Args>
  explicit doodle_error(fmt::format_string<Args...> fmt_str, Args&&... in_args)
      : std::runtime_error(fmt::format(std::forward<fmt::format_string<Args...>>(fmt_str), std::forward<Args>(in_args)...)),
        error_code_(0){};
  template <typename... Args>
  explicit doodle_error(std::int32_t in_core, const std::string& fmt_str, Args&&... in_args)
      : std::runtime_error(fmt::vformat(fmt_str, fmt::make_format_args(std::forward<Args>(in_args)...))),
        error_code_(in_core){};
};

class DOODLE_CORE_API http_request_error : public std::runtime_error {
 public:
  boost::beast::http::status code_status_;
  explicit http_request_error(boost::beast::http::status in_status, const std::string& message)
      : code_status_{in_status}, std::runtime_error(message) {};
};

class DOODLE_CORE_API doodle_category : public bsys::error_category {
 public:
  const char* name() const noexcept final;

  std::string message(int ev) const final;

  bsys::error_condition default_error_condition(int ev) const noexcept final;

  static const bsys::error_category& get();
};

class DOODLE_CORE_API maya_code_category : public bsys::error_category {
 public:
  const char* name() const noexcept final;

  std::string message(int ev) const final;

  static const bsys::error_category& get();
};

template <typename exception_type>
[[noreturn]] inline void throw_exception(
    exception_type&& in_exception_type, ::boost::source_location const& in_loc = std::source_location::current()
) {
  boost::throw_exception(std::forward<exception_type>(in_exception_type), in_loc);
}

template <typename Error>
[[noreturn]] void throw_error(
    Error in_error_index, ::boost::source_location const& in_loc = std::source_location::current()
) {
  boost::throw_exception(std::system_error{bsys::error_code{in_error_index}}, in_loc);
}

template <typename Error>
[[noreturn]] void throw_error(
    Error in_error_index, const std::string& mess,
    ::boost::source_location const& in_loc = std::source_location::current()
) {
  boost::throw_exception(std::system_error{bsys::error_code{in_error_index}, mess}, in_loc);
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
 */
template <>
struct formatter<::doodle::doodle_error> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const ::doodle::doodle_error& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<string_view>::format(boost::diagnostic_information(in_), ctx);
  }
};
}  // namespace fmt

namespace boost::system {
template <>
struct DOODLE_CORE_API is_error_code_enum<::doodle::error_enum::error_t> : std::true_type {};
}  // namespace boost::system
namespace std {
template <>
struct DOODLE_CORE_API is_error_code_enum<::doodle::error_enum::error_t> : std::true_type {};
}  // namespace std
namespace boost::system {
template <>
struct DOODLE_CORE_API is_error_code_enum<::doodle::maya_enum::maya_error_t> : std::true_type {};
}  // namespace boost::system
namespace std {
template <>
struct DOODLE_CORE_API is_error_code_enum<::doodle::maya_enum::maya_error_t> : std::true_type {};
}  // namespace std