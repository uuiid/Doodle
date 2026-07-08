#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/bind_value.h>
#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/session.h>

#include <initializer_list>
#include <memory>
#include <range/v3/view/unique.hpp>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

namespace doodle::orm {
enum class compare_operator {
  and_,
  or_,
  // == operators
  equal,
  // != operators
  not_equal,
};

// and 运算符
struct operator_compare_t : public column_operations_base_t {
  struct data_impl {
    compare_operator op_;
    std::shared_ptr<column_operations_base_t> left_;
    std::shared_ptr<column_operations_base_t> right_;
    bind_value_collector_t cached_variants_;
  };
  std::shared_ptr<data_impl> data_impl_ptr_;
  operator_compare_t();

  std::string to_sql(const session& s, const to_sql_ctx& ctx) const override;
  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
  // std::string (const session& s, const to_sql_ctx& ctx) const override;

  // operator &&, || 需要返回一个新的 operator_compare_t 对象，包含新的 SQL 片段和绑定函数
  operator_compare_t operator&&(operator_compare_t&& other) const;
  operator_compare_t operator||(operator_compare_t&& other) const;

  // operator &&, || column_operations
  template <typename U>
    requires(std::is_base_of_v<column_operations_base_t, std::decay_t<U>>)
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
    requires(std::is_base_of_v<column_operations_base_t, std::decay_t<U>>)
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
// 表达式类辅助, 消除 column_operations 中 operator= 成为自身复制函数的歧义
struct expression_t {
  std::shared_ptr<column_operations> column_op_;
};
// 约束, 可以用于 bind_value_t 构造
template <typename T>
concept is_bindable_value =
    !std::is_base_of_v<column_operations, std::decay_t<T>> && !is_alias_column_t_v<std::decay_t<T>> &&
    !std::is_same_v<std::decay_t<T>, expression_t> && !std::is_same_v<std::decay_t<T>, bind_value_t>;

struct column_operations : column_operations_base_t {
 private:
  struct to_str_base_t {
    ~to_str_base_t()                                                                                   = default;
    virtual std::string to_str(column_info_ptr& in_ptr, const session& s, const to_sql_ctx& ctx) const = 0;
    virtual void collect_bind_variants(bind_value_collector_t& bind_variants) const                    = 0;
  };
  struct to_str_value_t : to_str_base_t {
    std::string fmt_str_;
    bind_value_t value_variant_;
    explicit to_str_value_t(std::string fmt_str);
    std::string to_str(column_info_ptr& in_ptr, const session& s, const to_sql_ctx& ctx) const override;
    void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
  };
  struct to_str_value_list_t : to_str_base_t {
    std::string fmt_str_;
    std::vector<bind_value_t> value_variants_;
    explicit to_str_value_list_t(std::string fmt_str);
    std::string to_str(column_info_ptr& in_ptr, const session& s, const to_sql_ctx& ctx) const override;
    void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
  };

  struct to_str_subquery_t : to_str_base_t {
    std::shared_ptr<select_t> subquery_ptr_;
    bool is_not_in_{false};  // 标记是 IN 还是 NOT IN
    explicit to_str_subquery_t(std::shared_ptr<select_t> subquery_ptr);
    std::string to_str(column_info_ptr& in_ptr, const session& s, const to_sql_ctx& ctx) const override;
    void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
  };
  // NEW.uuid = OLD.uuid   NEW.name != OLD.name 别名比较, 必须包含表名以避免歧义
  struct to_str_compare_t : to_str_base_t {
    std::string fmt_str_;
    column_info_ptr other_column_ptr_;
    explicit to_str_compare_t(std::string fmt_str, column_info_ptr other_column_ptr);
    std::string to_str(column_info_ptr& in_ptr, const session& s, const to_sql_ctx& ctx) const override;
    void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
  };
  // 这里的 column_to_str 主要是将 column_operations 转换为对应的列名称，用于生成 SQL 片段
  struct column_to_str : to_str_base_t {
    explicit column_to_str() = default;
    std::string to_str(column_info_ptr& in_ptr, const session& s, const to_sql_ctx& ctx) const override;
    void collect_bind_variants(bind_value_collector_t& /*bind_variants*/) const override {
      // column_to_str 不包含绑定参数，因此不需要收集
    }
  };
  // 使用表达式转换
  struct to_str_expr_t : to_str_base_t {
    std::string fmt_str_;
    column_operations_ptr left_;
    explicit to_str_expr_t(std::string fmt_str, column_operations_ptr left)
        : fmt_str_(std::move(fmt_str)), left_(std::move(left)) {}
    std::string to_str(column_info_ptr& in_ptr, const session& s, const to_sql_ctx& ctx) const override;
    void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
  };

  struct data_impl {
    column_info_ptr ptr_shared_;
    std::shared_ptr<to_str_base_t> to_str_ptr_;
    bool is_set_operation_{false};  // 标记是否是 SET 操作，影响 SQL 片段的生成
  };
  std::shared_ptr<data_impl> data_impl_ptr_;

 public:
  template <typename T>
  explicit column_operations(auto T::* in_ptr);
  explicit column_operations(const table_columns_t& in_column);
  explicit column_operations(const alias_column_info_t& in_column);
  explicit column_operations(any_column_info_t in_any_column);
  explicit column_operations(rowid_column_info_t in_rowid_column);

  column_info_ptr get_column_info_ptr() const;

  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;

  std::string to_sql(const session& s, const to_sql_ctx& ctx) const override;

  // std::string (const session& s, const to_sql_ctx& ctx) const override;

  // 赋值操作符，生成SQL片段和绑定函数
  template <typename U>
    requires(is_alias_column_t_v<std::decay_t<U>>)
  column_operations operator=(U&& value) const {
    data_impl_ptr_->to_str_ptr_ =
        std::make_shared<to_str_compare_t>("{} = {}", std::make_shared<alias_column_info_t>(std::forward<U>(value)));
    data_impl_ptr_->is_set_operation_ = true;  // 标记为 SET 操作
    return *this;
  }
  template <typename U>
    requires(is_bindable_value<U>)
  column_operations operator=(U&& value) const {
    auto l_ptr                        = std::make_shared<to_str_value_t>("{} = ?");
    l_ptr->value_variant_             = bind_value_t{std::forward<U>(value)};
    data_impl_ptr_->to_str_ptr_       = l_ptr;
    data_impl_ptr_->is_set_operation_ = true;  // 标记为 SET 操作
    return *this;
  }

  column_operations operator=(bind_value_t&& value) const;
  column_operations operator=(std::nullptr_t) const;
  column_operations operator=(const expression_t& other) const;

  template <typename U>
    requires(is_alias_column_t_v<std::decay_t<U>>)
  auto operator==(U&& value) const {
    data_impl_ptr_->to_str_ptr_ =
        std::make_shared<to_str_compare_t>("{} == {}", std::make_shared<alias_column_info_t>(std::forward<U>(value)));
    return *this;
  }
  template <typename U>
    requires(std::is_same_v<std::decay_t<U>, bind_value_t>)
  auto operator==(U&& value) const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("{} == ?");
    l_ptr->value_variant_       = value;
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  template <typename U>
    requires(is_bindable_value<U>)
  auto operator==(U&& value) const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("{} == ?");
    l_ptr->value_variant_       = bind_value_t{std::forward<U>(value)};
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  template <typename U>
    requires(std::is_base_of_v<column_operations, std::decay_t<U>>)
  auto operator==(U&& other) const {
    auto l_self_ptr  = std::make_shared<column_operations>(std::move(*this));
    auto l_other_ptr = std::make_shared<std::decay_t<U>>(std::forward<U>(other));
    operator_compare_t compare{};
    compare.data_impl_ptr_->op_    = compare_operator::equal;  // 这里使用 EQUAL 来连接两个条件
    compare.data_impl_ptr_->left_  = l_self_ptr;
    compare.data_impl_ptr_->right_ = l_other_ptr;
    return compare;
  }
  auto operator==(std::nullptr_t) const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("{} IS NULL");
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }

  template <typename U>
    requires(is_alias_column_t_v<std::decay_t<U>>)
  auto operator!=(U&& value) const {
    data_impl_ptr_->to_str_ptr_ =
        std::make_shared<to_str_compare_t>("{} != {}", std::make_shared<alias_column_info_t>(std::forward<U>(value)));
    return *this;
  }
  template <typename U>
    requires(std::is_same_v<std::decay_t<U>, bind_value_t>)
  auto operator!=(U&& value) const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("{} != ?");
    l_ptr->value_variant_       = value;
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  template <typename U>
    requires(is_bindable_value<U>)
  auto operator!=(U&& value) const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("{} != ?");
    l_ptr->value_variant_       = bind_value_t{std::forward<U>(value)};
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  template <typename U>
    requires(std::is_base_of_v<column_operations, std::decay_t<U>>)
  auto operator!=(U&& other) const {
    auto l_self_ptr  = std::make_shared<column_operations>(std::move(*this));
    auto l_other_ptr = std::make_shared<std::decay_t<U>>(std::forward<U>(other));
    operator_compare_t compare{};
    compare.data_impl_ptr_->op_    = compare_operator::not_equal;  // 这里使用 NOT_EQUAL 来连接两个条件
    compare.data_impl_ptr_->left_  = l_self_ptr;
    compare.data_impl_ptr_->right_ = l_other_ptr;
    return compare;
  }
  auto operator!=(std::nullptr_t) const {
    auto l_ptr                  = std::make_shared<to_str_value_t>("{} IS NOT NULL");
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }

  // operator >, <, >=, <=
  template <typename U>
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
  auto operator>(U&& value) const {
    if constexpr (is_alias_column_t_v<std::decay_t<U>>) {
      data_impl_ptr_->to_str_ptr_ =
          std::make_shared<to_str_compare_t>("{} > {}", std::make_shared<alias_column_info_t>(std::forward<U>(value)));
    } else {
      auto l_ptr                  = std::make_shared<to_str_value_t>("{} > ?");
      l_ptr->value_variant_       = bind_value_t{std::forward<U>(value)};
      data_impl_ptr_->to_str_ptr_ = l_ptr;
    }
    return *this;
  }
  template <typename U>
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
  auto operator<(U&& value) const {
    if constexpr (is_alias_column_t_v<std::decay_t<U>>) {
      data_impl_ptr_->to_str_ptr_ =
          std::make_shared<to_str_compare_t>("{} < {}", std::make_shared<alias_column_info_t>(std::forward<U>(value)));
    } else {
      auto l_ptr                  = std::make_shared<to_str_value_t>("{} < ?");
      l_ptr->value_variant_       = bind_value_t{std::forward<U>(value)};
      data_impl_ptr_->to_str_ptr_ = l_ptr;
    }
    return *this;
  }
  template <typename U>
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
  auto operator>=(U&& value) const {
    if constexpr (is_alias_column_t_v<std::decay_t<U>>) {
      data_impl_ptr_->to_str_ptr_ =
          std::make_shared<to_str_compare_t>("{} >= {}", std::make_shared<alias_column_info_t>(std::forward<U>(value)));
    } else {
      auto l_ptr                  = std::make_shared<to_str_value_t>("{} >= ?");
      l_ptr->value_variant_       = bind_value_t{std::forward<U>(value)};
      data_impl_ptr_->to_str_ptr_ = l_ptr;
    }
    return *this;
  }
  template <typename U>
    requires(!std::is_base_of_v<column_operations, std::decay_t<U>>)
  auto operator<=(U&& value) const {
    if constexpr (is_alias_column_t_v<std::decay_t<U>>) {
      data_impl_ptr_->to_str_ptr_ =
          std::make_shared<to_str_compare_t>("{} <= {}", std::make_shared<alias_column_info_t>(std::forward<U>(value)));
    } else {
      auto l_ptr                  = std::make_shared<to_str_value_t>("{} <= ?");
      l_ptr->value_variant_       = bind_value_t{std::forward<U>(value)};
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
    requires std::ranges::range<std::decay_t<Container>> && (!std::is_same_v<std::decay_t<Container>, std::string>)
  auto in(const Container& values) const {
    auto l_size = std::ranges::distance(values);
    if (l_size == 0) {
      auto l_ptr                  = std::make_shared<to_str_value_t>("{} IN (NULL)");
      data_impl_ptr_->to_str_ptr_ = l_ptr;
      return *this;
    }
    std::vector<char> placeholders(l_size, '?');
    auto l_ptr = std::make_shared<to_str_value_list_t>(fmt::format("{{}} IN ({})", fmt::join(placeholders, ", ")));
    for (const auto& value : values) l_ptr->value_variants_.push_back(bind_value_t{value});
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  // operator in with initializer_list
  template <typename T>
  column_operations in(std::initializer_list<T> values) const {
    return in<std::initializer_list<T>>(values);
  }
  // operator in with subquery
  column_operations in(const select_t& subquery) const;

  // operator not in with Container
  template <typename Container>
    requires std::ranges::range<std::decay_t<Container>> && (!std::is_same_v<std::decay_t<Container>, std::string>)
  auto not_in(const Container& values) const {
    auto l_size = std::ranges::distance(values);
    if (l_size == 0) {
      auto l_ptr                  = std::make_shared<to_str_value_t>("{} NOT IN (NULL)");
      data_impl_ptr_->to_str_ptr_ = l_ptr;
      return *this;
    }
    std::vector<char> placeholders(l_size, '?');
    auto l_ptr = std::make_shared<to_str_value_list_t>(fmt::format("{{}} NOT IN ({})", fmt::join(placeholders, ", ")));
    for (const auto& value : values) l_ptr->value_variants_.push_back(bind_value_t{value});
    data_impl_ptr_->to_str_ptr_ = l_ptr;
    return *this;
  }
  // operator not in with initializer_list
  template <typename T>
  column_operations not_in(std::initializer_list<T> values) const {
    return not_in<std::initializer_list<T>>(values);
  }
  // operator not in with subquery
  column_operations not_in(const select_t& subquery) const;

  // fts5 match 语法
  column_operations match(std::string pattern) const;

  // operator and, or
  operator_compare_t operator&&(column_operations&& other) const;
  operator_compare_t operator||(column_operations&& other) const;

  // + - * / % 等算术运算符
  expression_t operator+(const std::int64_t& value) const;
  expression_t operator-(const std::int64_t& value) const;
  expression_t operator*(const std::int64_t& value) const;
  expression_t operator/(const std::int64_t& value) const;
  expression_t operator%(const std::int64_t& value) const;
};

// 动态查询
struct dynamic_column_operations : column_operations_base_t {
  std::vector<std::shared_ptr<column_operations_base_t>> operations_;
  dynamic_column_operations();
  std::string to_sql(const session& s, const to_sql_ctx& ctx) const override;
  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
  // std::string (const session& s, const to_sql_ctx& ctx) const override;

  template <typename T>
  void add_condition(T&& condition) {
    operations_.push_back(std::make_shared<std::decay_t<T>>(std::forward<T>(condition)));
  }
};

struct on_operations : column_operations_base_t {
  std::shared_ptr<column_operations_base_t> expr_;
  on_operations();
  template <typename T>
    requires std::derived_from<std::decay_t<T>, column_operations_base_t>
  on_operations(T&& condition) : expr_(std::make_shared<std::decay_t<T>>(std::forward<T>(condition))) {}

  std::string to_sql(const session& s, const to_sql_ctx& ctx) const override;
  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
};

// exists 语法
struct exists_operations : column_operations_base_t {
  std::shared_ptr<select_t> subquery_ptr_;
  exists_operations();
  explicit exists_operations(std::shared_ptr<select_t> subquery_ptr);
  std::string to_sql(const session& s, const to_sql_ctx& ctx) const override;
  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
};

inline auto exists(const select_t& subquery) { return exists_operations(std::make_shared<select_t>(subquery)); }

template <typename T>
auto on(T&& condition) {
  return on_operations(std::forward<T>(condition));
}

template <typename T>
auto c(auto T::* in_ptr) {
  return column_operations{in_ptr};
}
template <typename T>
  requires is_alias_column_t_v<T>
auto c(T&& in_column) {
  return column_operations{std::forward<T>(in_column)};
}
inline auto c(any_column_info_t in_any_column) { return column_operations{std::move(in_any_column)}; }
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
      case doodle::orm::compare_operator::equal:
        name = "==";
        break;
      case doodle::orm::compare_operator::not_equal:
        name = "!=";
        break;
    }
    format_to(ctx.out(), "{}", name);
    return ctx.out();
  }
};
}  // namespace fmt