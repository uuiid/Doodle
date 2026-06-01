#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle::orm {
class sqlite_orm_exception : public std::runtime_error {
 public:
  explicit sqlite_orm_exception(const std::string& message) : std::runtime_error(message) {}
  // explicit sqlite_orm_exception(std::int32_t error_code)
  // : std::runtime_error(fmt::format("SQLite error code: {}", error_code)), error_code_(error_code) {}
};

// 重新bing range参数时, 参数数量必须与set_range时一致, 否则抛出异常
struct rebind_range_size_mismatch_exception : public sqlite_orm_exception {
  rebind_range_size_mismatch_exception() : sqlite_orm_exception("rebind_range中的值数量必须与set_range时一致") {}
};
}  // namespace doodle::orm