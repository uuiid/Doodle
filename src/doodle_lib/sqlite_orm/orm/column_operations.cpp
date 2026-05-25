#include "column_operations.h"

#include <doodle_lib/sqlite_orm/orm/bind_value.h>
#include <doodle_lib/sqlite_orm/orm/bind_value_impl.h>
#include <doodle_lib/sqlite_orm/orm/storage_impl.h>

#include <fmt/format.h>
#include <vector>

namespace doodle::orm {

// operator_compare_t
operator_compare_t::operator_compare_t() : data_impl_ptr_(std::make_shared<data_impl>()) {}

std::string operator_compare_t::to_sql(const storage& s, to_sql_ctx ctx) const {
  auto l_ctx = ctx;
  l_ctx.ctx_ |= to_sql_ctx::where_sql;  // 强制使用 where_sql 上下文，以确保生成正确的 SQL 片段格式
  return fmt::format(
      "({} {} {})", data_impl_ptr_->left_->to_sql(s, l_ctx), data_impl_ptr_->op_,
      data_impl_ptr_->right_->to_sql(s, l_ctx)
  );
}

void operator_compare_t::collect_bind_variants(bind_value_collector_t& bind_variants) const {
  data_impl_ptr_->left_->collect_bind_variants(bind_variants);
  data_impl_ptr_->right_->collect_bind_variants(bind_variants);
}

// std::string operator_compare_t::get_column_name(const storage& /*s*/, to_sql_ctx ctx) const {
//   // 直接抛出异常，因为 operator_compare_t 不代表一个具体的列，无法生成列名
//   throw std::runtime_error("operator_compare_t does not represent a specific column and cannot generate a column
//   name");
// }

operator_compare_t operator_compare_t::operator&&(operator_compare_t&& other) const {
  operator_compare_t compare{};
  auto l_self_ptr                = std::make_shared<operator_compare_t>(std::move(*this));
  auto l_other_ptr               = std::make_shared<operator_compare_t>(other);
  compare.data_impl_ptr_->op_    = compare_operator::and_;
  compare.data_impl_ptr_->left_  = l_self_ptr;
  compare.data_impl_ptr_->right_ = l_other_ptr;
  return compare;
}

operator_compare_t operator_compare_t::operator||(operator_compare_t&& other) const {
  operator_compare_t compare{};
  auto l_self_ptr                = std::make_shared<operator_compare_t>(std::move(*this));
  auto l_other_ptr               = std::make_shared<operator_compare_t>(other);
  compare.data_impl_ptr_->op_    = compare_operator::or_;
  compare.data_impl_ptr_->left_  = l_self_ptr;
  compare.data_impl_ptr_->right_ = l_other_ptr;
  return compare;
}

// column_operations::to_str_value_t
column_operations::to_str_value_t::to_str_value_t(std::string fmt_str) : fmt_str_(std::move(fmt_str)) {}

std::string column_operations::to_str_value_t::to_str(column_info_ptr& in_ptr, const storage& s, to_sql_ctx ctx) const {
  auto l_column_name = in_ptr->get_column_name(s, ctx);
  return fmt::vformat(fmt_str_, fmt::make_format_args(l_column_name));
}

void column_operations::to_str_value_t::collect_bind_variants(bind_value_collector_t& bind_variants) const {
  if (value_variant_) bind_variants.bind_values_.push_back(value_variant_);
}

// column_operations::to_str_value_list_t
column_operations::to_str_value_list_t::to_str_value_list_t(std::string fmt_str) : fmt_str_(std::move(fmt_str)) {}

std::string column_operations::to_str_value_list_t::to_str(
    column_info_ptr& in_ptr, const storage& s, to_sql_ctx ctx
) const {
  auto l_column_name = in_ptr->get_column_name(s, ctx);
  return fmt::vformat(fmt_str_, fmt::make_format_args(l_column_name));
}

void column_operations::to_str_value_list_t::collect_bind_variants(bind_value_collector_t& bind_variants) const {
  bind_variants.bind_values_.insert(bind_variants.bind_values_.end(), value_variants_.begin(), value_variants_.end());
}

// column_operations::to_str_subquery_t
column_operations::to_str_subquery_t::to_str_subquery_t(std::shared_ptr<select_t> subquery_ptr)
    : subquery_ptr_(std::move(subquery_ptr)) {}

std::string column_operations::to_str_subquery_t::to_str(
    column_info_ptr& in_ptr, const storage& s, to_sql_ctx ctx
) const {
  auto l_column_name = in_ptr->get_column_name(s, ctx);
  if (is_not_in_) {
    return fmt::format("{} NOT IN ({})", l_column_name, subquery_ptr_->to_sql(ctx));
  } else {
    return fmt::format("{} IN ({})", l_column_name, subquery_ptr_->to_sql(ctx));
  }
}

void column_operations::to_str_subquery_t::collect_bind_variants(bind_value_collector_t& bind_variants) const {
  if (subquery_ptr_) subquery_ptr_->collect_bind_variants(bind_variants);
}

// column_operations::to_str_compare_t
column_operations::to_str_compare_t::to_str_compare_t(std::string fmt_str, column_info_ptr other_column_ptr)
    : fmt_str_(std::move(fmt_str)), other_column_ptr_(std::move(other_column_ptr)) {}

std::string column_operations::to_str_compare_t::to_str(
    column_info_ptr& in_ptr, const storage& s, to_sql_ctx ctx
) const {
  auto l_column_name       = in_ptr->get_column_name(s, ctx);
  auto l_other_column_name = other_column_ptr_->get_column_name(s, ctx);
  return fmt::vformat(fmt_str_, fmt::make_format_args(l_column_name, l_other_column_name));
}

void column_operations::to_str_compare_t::collect_bind_variants(
    bind_value_collector_t& /*bind_variants*/
) const {
  // 不需要绑定参数，因为比较的值来自于另一个列，而不是用户输入的值
}

std::string column_operations::column_to_str::to_str(column_info_ptr& in_ptr, const storage& s, to_sql_ctx ctx) const {
  auto l_column_name = in_ptr->get_column_name(s, ctx);
  return l_column_name;
}

column_operations::column_operations(const table_columns_t& in_column) : data_impl_ptr_(std::make_shared<data_impl>()) {
  data_impl_ptr_->ptr_shared_ = std::make_shared<column_info_t>(in_column);
  data_impl_ptr_->to_str_ptr_ = std::make_shared<column_to_str>();
};
column_operations::column_operations(const alias_column_info_t& in_column)
    : data_impl_ptr_(std::make_shared<data_impl>()) {
  data_impl_ptr_->ptr_shared_ = std::make_shared<alias_column_info_t>(in_column);
  data_impl_ptr_->to_str_ptr_ = std::make_shared<column_to_str>();
}

// column_operations public methods
column_info_ptr column_operations::get_column_info_ptr() const { return data_impl_ptr_->ptr_shared_; }

void column_operations::collect_bind_variants(bind_value_collector_t& bind_variants) const {
  data_impl_ptr_->to_str_ptr_->collect_bind_variants(bind_variants);
}

std::string column_operations::to_sql(const storage& s, to_sql_ctx ctx) const {
  auto l_ctx = ctx;
  l_ctx.ctx_ |= to_sql_ctx::where_sql;  // 强制使用 where_sql 上下文，以确保生成正确的 SQL 片段格式
  return data_impl_ptr_->to_str_ptr_->to_str(data_impl_ptr_->ptr_shared_, s, l_ctx);
}

// std::string column_operations::get_column_name(const storage& s, to_sql_ctx ctx) const {
//   return data_impl_ptr_->ptr_shared_->get_column_name(s, ctx);
// }

column_operations column_operations::operator=(bind_value_t&& value) const {
  auto l_ptr                  = std::make_shared<to_str_value_t>("{} = ?");
  l_ptr->value_variant_       = std::move(value);
  data_impl_ptr_->to_str_ptr_ = l_ptr;
  return *this;
}

column_operations column_operations::operator=(std::nullptr_t) const {
  auto l_ptr                  = std::make_shared<to_str_value_t>("{} = NULL");
  data_impl_ptr_->to_str_ptr_ = l_ptr;
  return *this;
}

column_operations column_operations::operator!() const {
  auto l_ptr                  = std::make_shared<to_str_value_t>("NOT ({})");
  data_impl_ptr_->to_str_ptr_ = l_ptr;
  return *this;
}

column_operations column_operations::like(std::string_view pattern) const {
  auto l_ptr                  = std::make_shared<to_str_value_t>("{} LIKE ?");
  l_ptr->value_variant_       = bind_value_t{std::string(pattern)};
  data_impl_ptr_->to_str_ptr_ = l_ptr;
  return *this;
}

column_operations column_operations::in(const select_t& subquery) const {
  auto l_ptr                  = std::make_shared<to_str_subquery_t>(std::make_shared<select_t>(subquery));
  data_impl_ptr_->to_str_ptr_ = l_ptr;
  return *this;
}

column_operations column_operations::not_in(const select_t& subquery) const {
  auto l_ptr                  = std::make_shared<to_str_subquery_t>(std::make_shared<select_t>(subquery));
  l_ptr->is_not_in_           = true;  // 设置 NOT IN 标志
  data_impl_ptr_->to_str_ptr_ = l_ptr;
  return *this;
}

operator_compare_t column_operations::operator&&(column_operations&& other) const {
  operator_compare_t compare{};
  auto l_self_ptr                = std::make_shared<column_operations>(std::move(*this));
  auto l_other_ptr               = std::make_shared<column_operations>(other);
  compare.data_impl_ptr_->op_    = compare_operator::and_;
  compare.data_impl_ptr_->left_  = l_self_ptr;
  compare.data_impl_ptr_->right_ = l_other_ptr;
  return compare;
}
operator_compare_t column_operations::operator||(column_operations&& other) const {
  operator_compare_t compare{};
  auto l_self_ptr                = std::make_shared<column_operations>(std::move(*this));
  auto l_other_ptr               = std::make_shared<column_operations>(other);
  compare.data_impl_ptr_->op_    = compare_operator::or_;
  compare.data_impl_ptr_->left_  = l_self_ptr;
  compare.data_impl_ptr_->right_ = l_other_ptr;
  return compare;
}

dynamic_column_operations::dynamic_column_operations() = default;
std::string dynamic_column_operations::to_sql(const storage& s, to_sql_ctx ctx) const {
  auto l_ctx = ctx;
  l_ctx.ctx_ |= to_sql_ctx::where_sql;  // 强制使用 where_sql 上下文，以确保生成正确的 SQL 片段格式
  std::vector<std::string> l_sql_parts{};
  for (const auto& operation : operations_) {
    l_sql_parts.push_back(fmt::format("({})", operation->to_sql(s, l_ctx)));
  }

  return l_sql_parts.empty()
             ? "TRUE"
             : fmt::format("({})", fmt::join(l_sql_parts, " AND "));  // 如果没有条件，返回一个永远为真的条件
}
void dynamic_column_operations::collect_bind_variants(bind_value_collector_t& bind_variants) const {
  for (const auto& operation : operations_) {
    operation->collect_bind_variants(bind_variants);
  }
}
// std::string dynamic_column_operations::get_column_name(const storage& /*s*/, to_sql_ctx ctx) const {
//   // 直接抛出异常，因为 dynamic_column_operations 不代表一个具体的列，无法生成列名
//   throw std::runtime_error(
//       "dynamic_column_operations does not represent a specific column and cannot generate a column name"
//   );
// }

on_operations::on_operations() = default;
std::string on_operations::to_sql(const storage& s, to_sql_ctx ctx) const {
  auto l_ctx = ctx;
  l_ctx.ctx_ |= to_sql_ctx::where_sql;  // 强制使用 where_sql 上下文，以确保生成正确的 SQL 片段格式
  if (expr_) {
    return fmt::format("ON {}", expr_->to_sql(s, l_ctx));
  } else {
    return "ON 1=1";  // 默认返回一个永远为真的条件
  }
}
void on_operations::collect_bind_variants(bind_value_collector_t& bind_variants) const {
  if (expr_) expr_->collect_bind_variants(bind_variants);
}
// std::string on_operations::get_column_name(const storage& /*s*/, to_sql_ctx ctx) const {
//   // 直接抛出异常，因为 on_operations 不代表一个具体的列，无法生成列名
//   throw std::runtime_error("on_operations does not represent a specific column and cannot generate a column name");
// }

match_operations::match_operations(std::string pattern) : pattern_(std::move(pattern)) {}
std::string match_operations::to_sql(const storage& s, to_sql_ctx ctx) const {
  // auto column_name = get_column_name(s, ctx);
  return "MATCH ?";  // MATCH 操作符的 SQL 片段，具体的列名会在绑定参数时处理
}
void match_operations::collect_bind_variants(bind_value_collector_t& bind_variants) const {
  bind_variants.bind_values_.push_back(pattern_);
}
// std::string match_operations::get_column_name(const storage& s, to_sql_ctx ctx) const {
//   throw std::runtime_error("match_operations does not represent a specific column and cannot generate a column
//   name");
// }

operator_compare_t match_operations::operator&&(column_operations&& other) const {
  operator_compare_t compare{};
  auto l_self_ptr                = std::make_shared<match_operations>(std::move(*this));
  auto l_other_ptr               = std::make_shared<column_operations>(std::move(other));
  compare.data_impl_ptr_->op_    = compare_operator::and_;
  compare.data_impl_ptr_->left_  = l_self_ptr;
  compare.data_impl_ptr_->right_ = l_other_ptr;
  return compare;
}
operator_compare_t match_operations::operator||(column_operations&& other) const {
  operator_compare_t compare{};
  auto l_self_ptr                = std::make_shared<match_operations>(std::move(*this));
  auto l_other_ptr               = std::make_shared<column_operations>(std::move(other));
  compare.data_impl_ptr_->op_    = compare_operator::or_;
  compare.data_impl_ptr_->left_  = l_self_ptr;
  compare.data_impl_ptr_->right_ = l_other_ptr;
  return compare;
}

}  // namespace doodle::orm