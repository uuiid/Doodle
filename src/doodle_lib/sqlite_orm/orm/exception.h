#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle::orm {
class sqlite_orm_exception : public std::runtime_error {
 public:
  explicit sqlite_orm_exception(const std::string& message) : std::runtime_error(message) {}
  // explicit sqlite_orm_exception(std::int32_t error_code)
  // : std::runtime_error(fmt::format("SQLite error code: {}", error_code)), error_code_(error_code) {}
};
}  // namespace doodle::orm