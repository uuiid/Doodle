#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
template <typename T>
struct column_operations;

template <typename T>
struct is_column_operations_specialization : std::false_type {};

template <typename T>
struct is_column_operations_specialization<column_operations<T>> : std::true_type {};

template <typename T>
inline constexpr bool is_column_operations_specialization_v =
    is_column_operations_specialization<std::remove_cvref_t<T>>::value;
template <typename T>
struct column_operations {
  using column_ptr_type = table_columns_t<T>;
  column_ptr_type ptr_;
  std::type_index type_index_{typeid(T)};
  mutable std::function<std::string(const storage&)> to_sql_;

  column_operations(auto T::* in_ptr) : ptr_(in_ptr) {}

  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator==(U&& value) const {
    to_sql_ = [this, value = std::forward<U>(value)](const storage& s) {
      return fmt::format("{} = ?", s.get_column_name(ptr_));
    };
    return *this;
  }
};
template <typename T>
auto c(auto T::* in_ptr) {
  return column_operations<T>{in_ptr};
}
}  // namespace doodle::orm