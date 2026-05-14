#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
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

  std::string to_sql(const storage& s, bool include_table_name) const override;
  void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const override;
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
  struct to_str_base_t {
    ~to_str_base_t()                                                                                     = default;
    virtual std::string to_str(column_ptr_type& in_ptr, const storage& s, bool include_table_name) const = 0;
    virtual void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const = 0;
  };
  struct to_str_value_t : to_str_base_t {
    std::string fmt_str_;
    std::shared_ptr<storage_column_variant> value_variant_;
    explicit to_str_value_t(std::string fmt_str) : fmt_str_(std::move(fmt_str)) {}

    std::string to_str(column_ptr_type& in_ptr, const storage& s, bool include_table_name) const override {
      auto column_name = s.template get_column_name<T>(in_ptr, include_table_name);
      return fmt::vformat(fmt_str_, fmt::make_format_args(column_name));
    }
    void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const override {
      if (value_variant_) bind_variants.push_back(value_variant_);
    }
  };
  struct to_str_value_list_t : to_str_base_t {
    std::string fmt_str_;
    std::vector<std::shared_ptr<storage_column_variant>> value_variants_;
    explicit to_str_value_list_t(std::string fmt_str) : fmt_str_(std::move(fmt_str)) {}

    std::string to_str(column_ptr_type& in_ptr, const storage& s, bool include_table_name) const override {
      auto column_name = s.template get_column_name<T>(in_ptr, include_table_name);
      return fmt::vformat(fmt_str_, fmt::make_format_args(column_name));
    }
    void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const override {
      bind_variants.insert(bind_variants.end(), value_variants_.begin(), value_variants_.end());
    }
  };

  struct to_str_subquery_t : to_str_base_t {
    std::shared_ptr<select_t> subquery_ptr_;
    explicit to_str_subquery_t(std::shared_ptr<select_t> subquery_ptr) : subquery_ptr_(std::move(subquery_ptr)) {}

    std::string to_str(column_ptr_type& in_ptr, const storage& s, bool include_table_name) const override {
      auto column_name = s.template get_column_name<T>(in_ptr, include_table_name);
      return fmt::format("{} IN ({})", column_name, subquery_ptr_->to_sql(s));
    }
    void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const override {
      if (subquery_ptr_) subquery_ptr_->collect_bind_variants(bind_variants);
    }
  };
  // NEW.uuid = OLD.uuid   NEW.name != OLD.name
  struct to_str_compare_t : to_str_base_t {
    std::string fmt_str_;
    std::shared_ptr<column_operations_base_t> other_column_ptr_;
    explicit to_str_compare_t(std::string fmt_str, std::shared_ptr<column_operations_base_t> other_column_ptr)
        : fmt_str_(std::move(fmt_str)), other_column_ptr_(std::move(other_column_ptr)) {}

    std::string to_str(column_ptr_type& in_ptr, const storage& s, bool include_table_name) const override {
      auto column_name = s.template get_column_name<T>(in_ptr, include_table_name);
      return fmt::vformat(
          fmt_str_, fmt::make_format_args(column_name, other_column_ptr_->to_sql(s, include_table_name))
      );
    }
    void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const override {
      // 不需要绑定参数，因为比较的值来自于另一个列，而不是用户输入的值
    }
  };

  struct data_impl {
    std::shared_ptr<column_ptr_type> ptr_shared_;
    std::shared_ptr<to_str_base_t> to_str_ptr_;
  };
  std::shared_ptr<data_impl> data_impl_ptr_;

 public:
  explicit column_operations(auto T::* in_ptr) : data_impl_ptr_(std::make_shared<data_impl>()) {
    data_impl_ptr_->ptr_shared_ = std::make_shared<column_ptr_type>(in_ptr);
  }
  explicit column_operations(const column_ptr_type& in_column) : data_impl_ptr_(std::make_shared<data_impl>()) {
    data_impl_ptr_->ptr_shared_ = std::make_shared<column_ptr_type>(in_column);
  }

  void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const override {
    data_impl_ptr_->to_str_ptr_->collect_bind_variants(bind_variants);
  }

  std::string to_sql(const storage& s, bool include_table_name) const override {
    data_impl_ptr_->to_str_ptr_->to_sql(*data_impl_ptr_->ptr_shared_, s, include_table_name);
  }

  std::string get_column_name(const storage& s) const override {
    return s.template get_column_name<T>(*data_impl_ptr_->ptr_shared_, false);
  }

  // 赋值操作符，生成SQL片段和绑定函数
  template <typename U>
  column_operations operator=(U&& value) const {
    if constexpr (!is_column_operations_specialization_v<std::decay_t<U>>) {
      auto l_ptr = std::make_shared<to_str_value_t>("{} = ?");
      if constexpr (std::is_convertible_v<U, std::string>) {
        l_ptr->value_variant_ = std::make_shared<storage_column_variant>(std::string{value});
      } else {
        l_ptr->value_variant_ = std::make_shared<storage_column_variant>(std::forward<U>(value));
      }
      data_impl_ptr_->to_str_ptr_ = l_ptr;
    } else {
      auto l_other_column_ptr     = std::make_shared<std::decay_t<U>>(std::forward<U>(value));
      auto l_ptr                  = std::make_shared<to_str_compare_t>("{} = {}", l_other_column_ptr);
      data_impl_ptr_->to_str_ptr_ = l_ptr;
    }
    return *this;
  }
  column_operations operator=(std::nullptr_t) const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("{} = NULL");
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }

  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator==(U&& value) const {
    auto l_ptr = std::make_shared<to_str_value_t>("{} == ?");
    // 如果是 char* 或 const char*，需要转换为 std::string 存储在 variant 中，否则会有生命周期问题
    if constexpr (std::is_convertible_v<U, std::string>) {
      l_ptr->value_variant_ = std::make_shared<storage_column_variant>(std::string{value});
    } else {
      l_ptr->value_variant_ = std::make_shared<storage_column_variant>(std::forward<U>(value));
    }
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator!=(U&& value) const {
    auto l_ptr = std::make_shared<to_str_value_t>("{} != ?");
    // 如果是 char* 或 const char*，需要转换为 std::string 存储在 variant 中，否则会有生命周期问题
    if constexpr (std::is_convertible_v<U, std::string>) {
      l_ptr->value_variant_ = std::make_shared<storage_column_variant>(std::string{value});
    } else {
      l_ptr->value_variant_ = std::make_shared<storage_column_variant>(std::forward<U>(value));
    }
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  // operator >, <, >=, <=
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator>(U&& value) const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("{} > ?");
    l_ptr->value_variant_       = std::make_shared<storage_column_variant>(std::forward<U>(value));
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator<(U&& value) const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("{} < ?");
    l_ptr->value_variant_       = std::make_shared<storage_column_variant>(std::forward<U>(value));
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator>=(U&& value) const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("{} >= ?");
    l_ptr->value_variant_       = std::make_shared<storage_column_variant>(std::forward<U>(value));
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  template <typename U>
    requires(!is_column_operations_specialization_v<U>)
  auto operator<=(U&& value) const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("{} <= ?");
    l_ptr->value_variant_       = std::make_shared<storage_column_variant>(std::forward<U>(value));
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  // operator !
  auto operator!() const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("NOT ({})");
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  // operator like
  auto like(std::string_view pattern) const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("{} LIKE ?");
    l_ptr->value_variant_       = std::make_shared<storage_column_variant>(std::string{pattern});
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  // operator in
  template <typename Container>
    requires std::ranges::range<Container> && (!std::is_same_v<std::decay_t<Container>, std::string>)
  auto in(const Container& values) const {
    auto l_size = std::ranges::distance(values);
    if (l_size == 0) {
      auto l_ptr                  = std::make_shared<to_str_value_t>("{} IN (NULL)");
      data_impl_ptr_->to_str_ptr_ = l_ptr;
      return *this;
    }
    std::vector<char> placeholders(l_size, '?');
    auto l_ptr = std::make_shared<to_str_value_list_t>(fmt::format("{{}} IN ({})", fmt::join(placeholders, ", ")));
    for (const auto& value : values) l_ptr->value_variants_.push_back(std::make_shared<storage_column_variant>(value));
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  // operator in with subquery
  auto in(const select_t& subquery) const {
    auto l_ptr                  = std::make_shared<to_str_subquery_t>(std::make_shared<select_t>(subquery));
    data_impl_ptr_->to_str_ptr_ = l_ptr;
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