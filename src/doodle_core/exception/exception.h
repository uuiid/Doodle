#pragma once

#include <doodle_lib/configure/config.h>
#include <doodle_lib_export.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <spdlog/common.h>

namespace doodle {
class DOODLELIB_API doodle_error : public std::runtime_error {
 public:
  explicit doodle_error(const std::string& message) : std::runtime_error(message){};
};
// iterators
class DOODLELIB_API error_iterator : public std::runtime_error {
 public:
  explicit error_iterator(const std::string& message) : std::runtime_error(message){};
};
// 空指针错误
class DOODLELIB_API nullptr_error : public doodle_error {
 public:
  explicit nullptr_error(const std::string& err) : doodle_error(err){};
};

// 序列化错误
class DOODLELIB_API serialization_error : public doodle_error {
 public:
  explicit serialization_error(const std::string& err) : doodle_error(err){};
};

class DOODLELIB_API component_error : public doodle_error {
 public:
  explicit component_error(const std::string& err) : doodle_error(err){};
};

// fileErr
class DOODLELIB_API file_error : public doodle_error {
 public:
  file_error(const std::string& message)
      : doodle_error(message){};
};
// doodl err
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
