#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace doodle::orm {
enum class compare_operator {
  and_,
  or_,

};

struct column_operations_base_t {
 protected:
  column_operations_base_t() = default;

 public:
  // to sql operator
  virtual std::string to_sql(const storage& s) const                                             = 0;
  // 创建bind参数
  virtual const std::vector<std::shared_ptr<storage_column_variant>>& get_value_variants() const = 0;
  virtual std::string get_column_name(const storage& s) const                                    = 0;
};

// and 运算符
struct operator_compare_t : public column_operations_base_t {
  struct data_impl {
    compare_operator op_;
    std::shared_ptr<column_operations_base_t> left_;
    std::shared_ptr<column_operations_base_t> right_;
    std::vector<std::shared_ptr<storage_column_variant>> cached_variants_;
  };
  std::shared_ptr<data_impl> data_impl_ptr_;
  operator_compare_t() : data_impl_ptr_(std::make_shared<data_impl>()) {}

  std::string to_sql(const storage& s) const override;
  const std::vector<std::shared_ptr<storage_column_variant>>& get_value_variants() const override;
  std::string get_column_name(const storage& s) const override {
    // 直接抛出异常，因为 operator_compare_t 不代表一个具体的列，无法生成列名
    throw std::runtime_error(
        "operator_compare_t does not represent a specific column and cannot generate a column name"
    );
  }

  // operator &&, || 需要返回一个新的 operator_compare_t 对象，包含新的 SQL 片段和绑定函数
  operator_compare_t operator&&(operator_compare_t&& other) const {
    operator_compare_t compare{};
    auto l_self_ptr                = std::make_shared<operator_compare_t>(std::move(*this));
    auto l_other_ptr               = std::make_shared<operator_compare_t>(other);
    compare.data_impl_ptr_->op_    = compare_operator::and_;
    compare.data_impl_ptr_->left_  = l_self_ptr;
    compare.data_impl_ptr_->right_ = l_other_ptr;
    return compare;
  }
  operator_compare_t operator||(operator_compare_t&& other) const {
    operator_compare_t compare{};
    auto l_self_ptr                = std::make_shared<operator_compare_t>(std::move(*this));
    auto l_other_ptr               = std::make_shared<operator_compare_t>(other);
    compare.data_impl_ptr_->op_    = compare_operator::or_;
    compare.data_impl_ptr_->left_  = l_self_ptr;
    compare.data_impl_ptr_->right_ = l_other_ptr;
    return compare;
  }
  // operator &&, || column_operations
  template <typename U>
    requires(is_column_operations_specialization_v<U>)
  operator_compare_t operator&&(U&& other) const {
    auto l_self_ptr  = std::make_shared<operator_compare_t>(std::move(*this));
    auto l_other_ptr = std::make_shared<std::decay_t<U>>(std::forward<U>(other));
    operator_compare_t compare{};
    compare.data_impl_ptr_->op_    = compare_operator::and_;
    compare.data_impl_ptr_->left_  = l_self_ptr;
    compare.data_impl_ptr_->right_ = l_other_ptr;
    return compare;
  }
  template <typename U>
    requires(is_column_operations_specialization_v<U>)
  operator_compare_t operator||(U&& other) const {
    auto l_self_ptr  = std::make_shared<operator_compare_t>(std::move(*this));
    auto l_other_ptr = std::make_shared<std::decay_t<U>>(std::forward<U>(other));
    operator_compare_t compare{};
    compare.data_impl_ptr_->op_    = compare_operator::or_;
    compare.data_impl_ptr_->left_  = l_self_ptr;
    compare.data_impl_ptr_->right_ = l_other_ptr;
    return compare;
  }
};

template <typename T>
struct column_operations : column_operations_base_t {
  using column_ptr_type = table_columns_t<T>;

 private:
  struct data_impl {
    std::shared_ptr<column_ptr_type> ptr_shared_;
    std::string fmt_str_;
    std::vector<std::shared_ptr<storage_column_variant>> value_variant_;
  };
  std::shared_ptr<data_impl> data_impl_ptr_;

 public:
  explicit column_operations(auto T::* in_ptr) : data_impl_ptr_(std::make_shared<data_impl>()) {
    data_impl_ptr_->ptr_shared_ = std::make_shared<column_ptr_type>(in_ptr);
  }
  explicit column_operations(const column_ptr_type& in_column) : data_impl_ptr_(std::make_shared<data_impl>()) {
    data_impl_ptr_->ptr_shared_ = std::make_shared<column_ptr_type>(in_column);
  }
  const std::vector<std::shared_ptr<storage_column_variant>>& get_value_variants() const override {
    return data_impl_ptr_->value_variant_;
  }

  std::string to_sql(const storage& s) const override {
    auto column_name = s.template get_column_name<T>(*data_impl_ptr_->ptr_shared_, false);
    return fmt::vformat(data_impl_ptr_->fmt_str_, fmt::make_format_args(column_name));
  }

  std::string get_column_name(const storage& s) const override {
    return s.template get_column_name<T>(*data_impl_ptr_->ptr_shared_, false);
  }

  // 赋值操作符，生成SQL片段和绑定函数
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  column_operations operator=(U&& value) const {
    data_impl_ptr_->fmt_str_ = "{} = ?";
    if constexpr (std::is_convertible_v<U, std::string>) {
      data_impl_ptr_->value_variant_.push_back(std::make_shared<storage_column_variant>(std::string{value}));
    } else {
      data_impl_ptr_->value_variant_.push_back(std::make_shared<storage_column_variant>(std::forward<U>(value)));
    }
    return *this;
  }
  column_operations operator=(std::nullptr_t) const {
    data_impl_ptr_->fmt_str_ = "{} = NULL";
    return *this;
  }

  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator==(U&& value) const {
    data_impl_ptr_->fmt_str_ = "{} == ?";
    // 如果是 char* 或 const char*，需要转换为 std::string 存储在 variant 中，否则会有生命周期问题
    if constexpr (std::is_convertible_v<U, std::string>) {
      data_impl_ptr_->value_variant_.push_back(std::make_shared<storage_column_variant>(std::string{value}));
    } else {
      data_impl_ptr_->value_variant_.push_back(std::make_shared<storage_column_variant>(std::forward<U>(value)));
    }
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator!=(U&& value) const {
    data_impl_ptr_->fmt_str_ = "{} != ?";
    // 如果是 char* 或 const char*，需要转换为 std::string 存储在 variant 中，否则会有生命周期问题
    if constexpr (std::is_convertible_v<U, std::string>) {
      data_impl_ptr_->value_variant_.push_back(std::make_shared<storage_column_variant>(std::string{value}));
    } else {
      data_impl_ptr_->value_variant_.push_back(std::make_shared<storage_column_variant>(std::forward<U>(value)));
    }
    return *this;
  }
  // operator >, <, >=, <=
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator>(U&& value) const {
    data_impl_ptr_->fmt_str_ = "{} > ?";
    data_impl_ptr_->value_variant_.push_back(std::make_shared<storage_column_variant>(std::forward<U>(value)));
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator<(U&& value) const {
    data_impl_ptr_->fmt_str_ = "{} < ?";
    data_impl_ptr_->value_variant_.push_back(std::make_shared<storage_column_variant>(std::forward<U>(value)));
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator>=(U&& value) const {
    data_impl_ptr_->fmt_str_ = "{} >= ?";
    data_impl_ptr_->value_variant_.push_back(std::make_shared<storage_column_variant>(std::forward<U>(value)));
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator<=(U&& value) const {
    data_impl_ptr_->fmt_str_ = "{} <= ?";
    data_impl_ptr_->value_variant_.push_back(std::make_shared<storage_column_variant>(std::forward<U>(value)));
    return *this;
  }
  // operator !
  auto operator!() const {
    data_impl_ptr_->fmt_str_ = "NOT ({})";
    return *this;
  }
  // operator like
  auto like(std::string_view pattern) const {
    data_impl_ptr_->fmt_str_ = "{} LIKE ?";
    data_impl_ptr_->value_variant_.push_back(std::make_shared<storage_column_variant>(std::string{pattern}));
    return *this;
  }
  // operator in
  template <typename Container>
    requires std::ranges::range<Container> && (!std::is_same_v<std::decay_t<Container>, std::string>)
  auto in(const Container& values) const {
    auto l_size = std::ranges::distance(values);
    if (l_size == 0) {
      data_impl_ptr_->fmt_str_ = "{} IN (NULL)";
      return *this;
    }
    std::vector<char> placeholders(l_size, '?');
    data_impl_ptr_->fmt_str_ = fmt::format("{{}} IN ({})", fmt::join(placeholders, ", "));
    for (const auto& value : values)
      data_impl_ptr_->value_variant_.push_back(std::make_shared<storage_column_variant>(value));

    return *this;
  }

  // operator and, or
  template <typename U>
    requires(is_column_operations_specialization_v<U>)
  auto operator&&(U&& other) const {
    auto l_self_ptr  = std::make_shared<column_operations>(std::move(*this));
    auto l_other_ptr = std::make_shared<std::decay_t<U>>(std::forward<U>(other));
    operator_compare_t compare{};
    compare.data_impl_ptr_->op_    = compare_operator::and_;
    compare.data_impl_ptr_->left_  = l_self_ptr;
    compare.data_impl_ptr_->right_ = l_other_ptr;
    return compare;
  }
  template <typename U>
    requires(is_column_operations_specialization_v<U>)
  auto operator||(U&& other) const {
    auto l_self_ptr  = std::make_shared<column_operations>(std::move(*this));
    auto l_other_ptr = std::make_shared<std::decay_t<U>>(std::forward<U>(other));
    operator_compare_t compare{};
    compare.data_impl_ptr_->op_    = compare_operator::or_;
    compare.data_impl_ptr_->left_  = l_self_ptr;
    compare.data_impl_ptr_->right_ = l_other_ptr;
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