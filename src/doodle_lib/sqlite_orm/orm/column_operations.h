#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {

// and 运算符
struct and_t {
  std::function<std::string(const storage&)> left_;
  std::function<std::string(const storage&)> right_;

  std::string operator()(const storage& s) const { return fmt::format("({} AND {})", left_(s), right_(s)); }
};

template <typename T>
struct column_operations {
  using column_ptr_type = table_columns_t<T>;
  column_ptr_type ptr_;
  std::type_index type_index_{typeid(T)};

 private:
  mutable std::function<std::string(const storage&)> to_sql_;

 public:
  column_operations(auto T::* in_ptr) : ptr_(in_ptr) {}

  // to sql operator
  std::string to_sql(const storage& s) const {
    if (!to_sql_) {
      throw std::runtime_error("No operation specified for this column");
    }
    return to_sql_(s);
  }
  // 创建bind参数
  void bind();

  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator==(U&& value) const {
    to_sql_ = [this, value = std::forward<U>(value)](const storage& s) {
      return fmt::format("{} == ?", s.get_column_name(ptr_));
    };
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator!=(U&& value) const {
    to_sql_ = [this, value = std::forward<U>(value)](const storage& s) {
      return fmt::format("{} != ?", s.get_column_name(ptr_));
    };
    return *this;
  }
  // operator >, <, >=, <=
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator>(U&& value) const {
    to_sql_ = [this, value = std::forward<U>(value)](const storage& s) {
      return fmt::format("{} > ?", s.get_column_name(ptr_));
    };
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator<(U&& value) const {
    to_sql_ = [this, value = std::forward<U>(value)](const storage& s) {
      return fmt::format("{} < ?", s.get_column_name(ptr_));
    };
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator>=(U&& value) const {
    to_sql_ = [this, value = std::forward<U>(value)](const storage& s) {
      return fmt::format("{} >= ?", s.get_column_name(ptr_));
    };
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator<=(U&& value) const {
    to_sql_ = [this, value = std::forward<U>(value)](const storage& s) {
      return fmt::format("{} <= ?", s.get_column_name(ptr_));
    };
    return *this;
  }
  // operator !
  auto operator!() const {
    to_sql_ = [this](const storage& s) { return fmt::format("NOT {}", s.get_column_name(ptr_)); };
    return *this;
  }
  // operator like
  auto like(std::string_view pattern) const {
    to_sql_ = [this](const storage& s) { return fmt::format("{} LIKE ?", s.get_column_name(ptr_)); };
    return *this;
  }
  // operator in
  template <typename Container>
    requires std::ranges::range<Container> && (!std::is_same_v<std::decay_t<Container>, std::string>)
  auto in(const Container& values) const {
    to_sql_ = [this](const storage& s) { return fmt::format("{} IN ?", s.get_column_name(ptr_)); };
    return *this;
  }
};
template <typename T>
auto c(auto T::* in_ptr) {
  return column_operations<T>{in_ptr};
}
}  // namespace doodle::orm