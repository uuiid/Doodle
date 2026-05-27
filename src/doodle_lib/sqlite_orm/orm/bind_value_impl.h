#pragma once

#include <doodle_lib/sqlite_orm/orm/bind_value.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
template <typename T>
  requires(!std::same_as<std::remove_cvref_t<T>, bind_value_t> && !char_pointer<T>)
bind_value_t::bind_value_t(T&& value) {
  using value_type = std::decay_t<T>;
  value_           = std::forward<T>(value);
  bind_fun_        = [](const bind_value_t& self, sqlite_stmt& stmt) {
    using actual_type = value_type;
    auto& value_ref   = std::any_cast<const actual_type&>(self.value_);
    stmt.bind<actual_type>(value_ref);
  };
  to_string_fun_ = [](const bind_value_t& self, storage& in_storage, to_sql_ctx ctx) {
    using actual_type = value_type;
    auto& value_ref   = std::any_cast<const actual_type&>(self.value_);
    return fmt::to_string(value_ref);
  };
}
template <typename T>
  requires char_pointer<T>
bind_value_t::bind_value_t(T&& value) {
  // 如果是 char* 或 const char*，需要转换为 std::string 存储在 variant 中，否则会有生命周期问题
  value_    = std::string(std::forward<T>(value));
  bind_fun_ = [](const bind_value_t& self, sqlite_stmt& stmt) {
    using actual_type = std::string;
    auto& str_value   = std::any_cast<const actual_type&>(self.value_);
    stmt.bind<actual_type>(str_value);
  };
  to_string_fun_ = [](const bind_value_t& self, storage& in_storage, to_sql_ctx ctx) {
    using actual_type = std::string;
    auto& str_value   = std::any_cast<const actual_type&>(self.value_);
    return str_value;
  };
}

}  // namespace doodle::orm