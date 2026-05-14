#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include "column.h"
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
  operator_compare_t();

  std::string to_sql(const storage& s, bool include_table_name) const override;
  void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const override;
  std::string get_column_name(const storage& s) const override;

  // operator &&, || 需要返回一个新的 operator_compare_t 对象，包含新的 SQL 片段和绑定函数
  operator_compare_t operator&&(operator_compare_t&& other) const;
  operator_compare_t operator||(operator_compare_t&& other) const;

  // operator &&, || column_operations
  template <typename U>
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

struct column_operations : column_operations_base_t {
 private:
  struct to_str_base_t {
    ~to_str_base_t()                                                                                     = default;
    virtual std::string to_str(column_info_ptr& in_ptr, const storage& s, bool include_table_name) const = 0;
    virtual void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const = 0;
  };
  struct to_str_value_t : to_str_base_t {
    std::string fmt_str_;
    std::shared_ptr<storage_column_variant> value_variant_;
    explicit to_str_value_t(std::string fmt_str);
    std::string to_str(column_info_ptr& in_ptr, const storage& s, bool include_table_name) const override;
    void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const override;
  };
  struct to_str_value_list_t : to_str_base_t {
    std::string fmt_str_;
    std::vector<std::shared_ptr<storage_column_variant>> value_variants_;
    explicit to_str_value_list_t(std::string fmt_str);
    std::string to_str(column_info_ptr& in_ptr, const storage& s, bool include_table_name) const override;
    void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const override;
  };

  struct to_str_subquery_t : to_str_base_t {
    std::shared_ptr<select_t> subquery_ptr_;
    explicit to_str_subquery_t(std::shared_ptr<select_t> subquery_ptr);
    std::string to_str(column_info_ptr& in_ptr, const storage& s, bool include_table_name) const override;
    void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const override;
  };
  // NEW.uuid = OLD.uuid   NEW.name != OLD.name 别名比较, 必须包含表名以避免歧义
  struct to_str_compare_t : to_str_base_t {
    std::string fmt_str_;
    column_info_ptr other_column_ptr_;
    explicit to_str_compare_t(std::string fmt_str, column_info_ptr other_column_ptr);
    std::string to_str(column_info_ptr& in_ptr, const storage& s, bool include_table_name) const override;
    void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const override;
  };

  struct data_impl {
    column_info_ptr ptr_shared_;
    std::shared_ptr<to_str_base_t> to_str_ptr_;
  };
  std::shared_ptr<data_impl> data_impl_ptr_;

 public:
  template <typename T>
  explicit column_operations(auto T::* in_ptr);
  template <typename T>
  explicit column_operations(const table_columns_t<T>& in_column);
  template <typename T>
  explicit column_operations(const alias_column_info_t<T>& in_column);

  column_info_ptr get_column_info_ptr() const;

  void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const override;

  std::string to_sql(const storage& s, bool include_table_name) const override;

  std::string get_column_name(const storage& s) const override;

  // 赋值操作符，生成SQL片段和绑定函数
  template <typename U>
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
  column_operations operator=(U&& value) const {
    if constexpr (is_alias_column_info_specialization_v<std::decay_t<U>>) {
      data_impl_ptr_->to_str_ptr_ =
          std::make_shared<to_str_compare_t>("{} = {}", std::make_shared<std::decay_t<U>>(std::forward<U>(value)));

    } else {
      auto l_ptr = std::make_shared<to_str_value_t>("{} = ?");
      if constexpr (std::is_convertible_v<U, std::string>) {
        l_ptr->value_variant_ = std::make_shared<storage_column_variant>(std::string{value});
      } else {
        l_ptr->value_variant_ = std::make_shared<storage_column_variant>(std::forward<U>(value));
      }
      data_impl_ptr_->to_str_ptr_ = l_ptr;
    }
    return *this;
  }

  column_operations operator=(std::nullptr_t) const;

  template <typename U>
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
  auto operator==(U&& value) const {
    if constexpr (is_alias_column_info_specialization_v<std::decay_t<U>>) {
      data_impl_ptr_->to_str_ptr_ =
          std::make_shared<to_str_compare_t>("{} == {}", std::make_shared<std::decay_t<U>>(std::forward<U>(value)));
    } else {
      auto l_ptr = std::make_shared<to_str_value_t>("{} == ?");
      // 如果是 char* 或 const char*，需要转换为 std::string 存储在 variant 中，否则会有生命周期问题
      if constexpr (std::is_convertible_v<U, std::string>) {
        l_ptr->value_variant_ = std::make_shared<storage_column_variant>(std::string{value});
      } else {
        l_ptr->value_variant_ = std::make_shared<storage_column_variant>(std::forward<U>(value));
      }
      data_impl_ptr_->to_str_ptr_ = l_ptr;
    }
    return *this;
  }
  template <typename U>
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
  auto operator!=(U&& value) const {
    if constexpr (is_alias_column_info_specialization_v<std::decay_t<U>>) {
      data_impl_ptr_->to_str_ptr_ =
          std::make_shared<to_str_compare_t>("{} != {}", std::make_shared<std::decay_t<U>>(std::forward<U>(value)));
    } else {
      auto l_ptr = std::make_shared<to_str_value_t>("{} != ?");
      // 如果是 char* 或 const char*，需要转换为 std::string 存储在 variant 中，否则会有生命周期问题
      if constexpr (std::is_convertible_v<U, std::string>) {
        l_ptr->value_variant_ = std::make_shared<storage_column_variant>(std::string{value});
      } else {
        l_ptr->value_variant_ = std::make_shared<storage_column_variant>(std::forward<U>(value));
      }
      data_impl_ptr_->to_str_ptr_ = l_ptr;
    }
    return *this;
  }
  // operator >, <, >=, <=
  template <typename U>
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
  auto operator>(U&& value) const {
    if constexpr (is_alias_column_info_specialization_v<std::decay_t<U>>) {
      data_impl_ptr_->to_str_ptr_ =
          std::make_shared<to_str_compare_t>("{} > {}", std::make_shared<std::decay_t<U>>(std::forward<U>(value)));
    } else {
      auto l_ptr                  = std::make_shared<to_str_value_t>("{} > ?");
      l_ptr->value_variant_       = std::make_shared<storage_column_variant>(std::forward<U>(value));
      data_impl_ptr_->to_str_ptr_ = l_ptr;
    }
    return *this;
  }
  template <typename U>
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
  auto operator<(U&& value) const {
    if constexpr (is_alias_column_info_specialization_v<std::decay_t<U>>) {
      data_impl_ptr_->to_str_ptr_ =
          std::make_shared<to_str_compare_t>("{} < {}", std::make_shared<std::decay_t<U>>(std::forward<U>(value)));
    } else {
      auto l_ptr                  = std::make_shared<to_str_value_t>("{} < ?");
      l_ptr->value_variant_       = std::make_shared<storage_column_variant>(std::forward<U>(value));
      data_impl_ptr_->to_str_ptr_ = l_ptr;
    }
    return *this;
  }
  template <typename U>
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
  auto operator>=(U&& value) const {
    if constexpr (is_alias_column_info_specialization_v<std::decay_t<U>>) {
      data_impl_ptr_->to_str_ptr_ =
          std::make_shared<to_str_compare_t>("{} >= {}", std::make_shared<std::decay_t<U>>(std::forward<U>(value)));
    } else {
      auto l_ptr                  = std::make_shared<to_str_value_t>("{} >= ?");
      l_ptr->value_variant_       = std::make_shared<storage_column_variant>(std::forward<U>(value));
      data_impl_ptr_->to_str_ptr_ = l_ptr;
    }
    return *this;
  }
  template <typename U>
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
  auto operator<=(U&& value) const {
    if constexpr (is_alias_column_info_specialization_v<std::decay_t<U>>) {
      data_impl_ptr_->to_str_ptr_ =
          std::make_shared<to_str_compare_t>("{} <= {}", std::make_shared<std::decay_t<U>>(std::forward<U>(value)));
    } else {
      auto l_ptr                  = std::make_shared<to_str_value_t>("{} <= ?");
      l_ptr->value_variant_       = std::make_shared<storage_column_variant>(std::forward<U>(value));
      data_impl_ptr_->to_str_ptr_ = l_ptr;
    }
    return *this;
  }
  // operator !
  column_operations operator!() const;
  // operator like
  column_operations like(std::string_view pattern) const;
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
  column_operations in(const select_t& subquery) const;

  // operator and, or
  template <typename U>
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
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
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
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
  return column_operations{in_ptr};
}
template <typename T>
auto c(const alias_column_info_t<T>& in_column) {
  return column_operations{in_column};
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