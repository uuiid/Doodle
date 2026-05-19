#pragma once

#include <doodle_lib/sqlite_orm/orm/fwd.h>

namespace doodle::orm {
struct bind_value_t {
  std::any value_;
  // 这个类是可复制的, 因此需要确保 bind_fun_ 的复制行为正确, 因此, 不可以捕获 this指针, 直接在参数中传递需要的值
  std::function<void(const bind_value_t&, sqlite_stmt& stmt)> bind_fun_;

  template <typename T>
    requires(!std::same_as<std::remove_cvref_t<T>, bind_value_t>)
  explicit bind_value_t(T&& value);
  bind_value_t()                               = default;
  bind_value_t(const bind_value_t&)            = default;
  bind_value_t(bind_value_t&&)                 = default;
  bind_value_t& operator=(const bind_value_t&) = default;
  bind_value_t& operator=(bind_value_t&&)      = default;
  // to bool
  operator bool() const { return bind_fun_ != nullptr; }

  void bind(sqlite_stmt& stmt) const;
};
struct bind_value_collector_t {
  std::vector<bind_value_t> bind_values_;
};

}  // namespace doodle::orm