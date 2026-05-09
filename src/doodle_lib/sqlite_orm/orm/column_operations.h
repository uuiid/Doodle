#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <functional>
#include <memory>
#include <utility>

namespace doodle::orm {
enum class compare_operator {
  and_,
  or_,

};
// and 运算符
struct operator_compare_t {
  compare_operator op_;
  std::function<std::string(const storage&)> left_;
  std::function<std::string(const storage&)> right_;
  std::function<void(sqlite_stmt&)> bind_left_;
  std::function<void(sqlite_stmt&)> bind_right_;

  std::string to_sql(const storage& s) const;
  void bind(sqlite_stmt& stmt) const;
  // operator &&, || 需要返回一个新的 operator_compare_t 对象，包含新的 SQL 片段和绑定函数
  operator_compare_t operator&&(operator_compare_t&& other) const {
    operator_compare_t compare{};
    auto l_self_ptr     = std::make_shared<operator_compare_t>(std::move(*this));
    auto l_other_ptr    = std::make_shared<operator_compare_t>(other);
    compare.op_         = compare_operator::and_;
    compare.left_       = [l_self_ptr](const storage& s) { return l_self_ptr->to_sql(s); };
    compare.right_      = [l_other_ptr](const storage& s) { return l_other_ptr->to_sql(s); };
    compare.bind_left_  = [l_self_ptr](sqlite_stmt& stmt) { l_self_ptr->bind(stmt); };
    compare.bind_right_ = [l_other_ptr](sqlite_stmt& stmt) { l_other_ptr->bind(stmt); };
    return compare;
  }
  operator_compare_t operator||(operator_compare_t&& other) const {
    operator_compare_t compare{};
    auto l_self_ptr     = std::make_shared<operator_compare_t>(std::move(*this));
    auto l_other_ptr    = std::make_shared<operator_compare_t>(other);
    compare.op_         = compare_operator::or_;
    compare.left_       = [l_self_ptr](const storage& s) { return l_self_ptr->to_sql(s); };
    compare.right_      = [l_other_ptr](const storage& s) { return l_other_ptr->to_sql(s); };
    compare.bind_left_  = [l_self_ptr](sqlite_stmt& stmt) { l_self_ptr->bind(stmt); };
    compare.bind_right_ = [l_other_ptr](sqlite_stmt& stmt) { l_other_ptr->bind(stmt); };
    return compare;
  }
  // operator &&, || column_operations
  template <typename U>
    requires(is_column_operations_specialization_v<U>)
  operator_compare_t operator&&(U&& other) const {
    auto l_self_ptr  = std::make_shared<operator_compare_t>(std::move(*this));
    auto l_other_ptr = std::make_shared<std::decay_t<U>>(std::forward<U>(other));
    operator_compare_t compare{};
    compare.op_         = compare_operator::and_;
    compare.left_       = [l_self_ptr](const storage& s) { return l_self_ptr->to_sql(s); };
    compare.right_      = [l_other_ptr](const storage& s) { return l_other_ptr->to_sql(s); };
    compare.bind_left_  = [l_self_ptr](sqlite_stmt& stmt) { l_self_ptr->bind(stmt); };
    compare.bind_right_ = [l_other_ptr](sqlite_stmt& stmt) { l_other_ptr->bind(stmt); };
    return compare;
  }
  template <typename U>
    requires(is_column_operations_specialization_v<U>)
  operator_compare_t operator||(U&& other) const {
    auto l_self_ptr  = std::make_shared<operator_compare_t>(std::move(*this));
    auto l_other_ptr = std::make_shared<std::decay_t<U>>(std::forward<U>(other));
    operator_compare_t compare{};
    compare.op_         = compare_operator::or_;
    compare.left_       = [l_self_ptr](const storage& s) { return l_self_ptr->to_sql(s); };
    compare.right_      = [l_other_ptr](const storage& s) { return l_other_ptr->to_sql(s); };
    compare.bind_left_  = [l_self_ptr](sqlite_stmt& stmt) { l_self_ptr->bind(stmt); };
    compare.bind_right_ = [l_other_ptr](sqlite_stmt& stmt) { l_other_ptr->bind(stmt); };
    return compare;
  }
};

template <typename T>
struct column_operations {
  using column_ptr_type = table_columns_t<T>;
  std::shared_ptr<column_ptr_type> ptr_shared_;

 private:
  mutable std::function<std::string(const storage&)> to_sql_;
  mutable std::function<void(sqlite_stmt&)> bind_;

 public:
  column_operations(auto T::* in_ptr) : ptr_shared_(std::make_shared<column_ptr_type>(in_ptr)) {}

  // to sql operator
  std::string to_sql(const storage& s) const {
    if (!to_sql_) {
      throw std::runtime_error("No operation specified for this column");
    }
    return to_sql_(s);
  }
  // 创建bind参数
  void bind(sqlite_stmt& stmt) const {
    if (bind_) bind_(stmt);
  }

  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator==(U&& value) const {
    to_sql_ = [ptr = ptr_shared_](const storage& s) {
      return fmt::format("{} == ?", s.template get_column_name<T>(*ptr));
    };
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator!=(U&& value) const {
    to_sql_ = [ptr = ptr_shared_](const storage& s) {
      return fmt::format("{} != ?", s.template get_column_name<T>(*ptr));
    };
    return *this;
  }
  // operator >, <, >=, <=
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator>(U&& value) const {
    to_sql_ = [ptr = ptr_shared_](const storage& s) {
      return fmt::format("{} > ?", s.template get_column_name<T>(*ptr));
    };
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator<(U&& value) const {
    to_sql_ = [ptr = ptr_shared_](const storage& s) {
      return fmt::format("{} < ?", s.template get_column_name<T>(*ptr));
    };
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator>=(U&& value) const {
    to_sql_ = [ptr = ptr_shared_](const storage& s) {
      return fmt::format("{} >= ?", s.template get_column_name<T>(*ptr));
    };
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator<=(U&& value) const {
    to_sql_ = [ptr = ptr_shared_](const storage& s) {
      return fmt::format("{} <= ?", s.template get_column_name<T>(*ptr));
    };
    return *this;
  }
  // operator !
  auto operator!() const {
    to_sql_ = [ptr = ptr_shared_](const storage& s) {
      return fmt::format("NOT {}", s.template get_column_name<T>(*ptr));
    };
    return *this;
  }
  // operator like
  auto like(std::string_view pattern) const {
    to_sql_ = [ptr = ptr_shared_](const storage& s) {
      return fmt::format("{} LIKE ?", s.template get_column_name<T>(*ptr));
    };
    return *this;
  }
  // operator in
  template <typename Container>
    requires std::ranges::range<Container> && (!std::is_same_v<std::decay_t<Container>, std::string>)
  auto in(const Container& values) const {
    to_sql_ = [ptr = ptr_shared_](const storage& s) {
      return fmt::format("{} IN ?", s.template get_column_name<T>(*ptr));
    };
    return *this;
  }

  // operator and, or
  template <typename U>
    requires(is_column_operations_specialization_v<U>)
  auto operator&&(U&& other) const {
    auto l_self_ptr  = std::make_shared<column_operations>(std::move(*this));
    auto l_other_ptr = std::make_shared<std::decay_t<U>>(std::forward<U>(other));
    operator_compare_t compare{};
    compare.op_         = compare_operator::and_;
    compare.left_       = [l_self_ptr](const storage& s) { return l_self_ptr->to_sql(s); };
    compare.right_      = [l_other_ptr](const storage& s) { return l_other_ptr->to_sql(s); };

    compare.bind_left_  = [l_self_ptr](sqlite_stmt& stmt) { l_self_ptr->bind(stmt); };
    compare.bind_right_ = [l_other_ptr](sqlite_stmt& stmt) { l_other_ptr->bind(stmt); };
    return compare;
  }
  template <typename U>
    requires(is_column_operations_specialization_v<U>)
  auto operator||(U&& other) const {
    auto l_self_ptr  = std::make_shared<column_operations>(std::move(*this));
    auto l_other_ptr = std::make_shared<std::decay_t<U>>(std::forward<U>(other));
    operator_compare_t compare{};
    compare.op_         = compare_operator::or_;
    compare.left_       = [l_self_ptr](const storage& s) { return l_self_ptr->to_sql(s); };
    compare.right_      = [l_other_ptr](const storage& s) { return l_other_ptr->to_sql(s); };

    compare.bind_left_  = [l_self_ptr](sqlite_stmt& stmt) { l_self_ptr->bind(stmt); };
    compare.bind_right_ = [l_other_ptr](sqlite_stmt& stmt) { l_other_ptr->bind(stmt); };
    return compare;
  }
};
template <typename T>
auto c(auto T::* in_ptr) {
  return column_operations<T>{in_ptr};
}
}  // namespace doodle::orm

namespace fmt {
template <>
struct formatter<doodle::orm::compare_operator> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(doodle::orm::compare_operator op, FormatContext& ctx) const -> decltype(ctx.out()) {
    std::string_view name;
    switch (op) {
      case doodle::orm::compare_operator::and_:
        name = "AND";
        break;
      case doodle::orm::compare_operator::or_:
        name = "OR";
        break;
    }
    format_to(ctx.out(), "{}", name);
    return ctx.out();
  }
};
}  // namespace fmt