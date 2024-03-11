//
// Created by TD on 2024/3/8.
//

#pragma once
#include <sqlpp11/alias_provider.h>
#include <sqlpp11/data_types/parameter_value.h>
#include <sqlpp11/detail/type_set.h>
#include <sqlpp11/expression_operators.h>
#include <sqlpp11/sqlite3/connection.h>
#include <sqlpp11/type_traits.h>
#include <sqlpp11/wrap_operand.h>
namespace sqlpp::sqlite3 {
template <typename ValueType, typename NameType>
struct excluded_t : public expression_operators<excluded_t<ValueType, NameType>, ValueType> {
  using _traits                            = make_traits<ValueType, tag::is_expression>;

  using _nodes                             = sqlpp::detail::type_vector<>;
  //  using _parameters                        = sqlpp::detail::type_vector<excluded_t>;
  using _can_be_null                       = std::true_type;
  using _is_literal_expression             = std::true_type;

  //  using _instance_t                        = member_t<NameType, ValueType>;

  excluded_t()                             = default;

  excluded_t(const excluded_t&)            = default;
  excluded_t(excluded_t&&)                 = default;
  excluded_t& operator=(const excluded_t&) = default;
  excluded_t& operator=(excluded_t&&)      = default;
  ~excluded_t()                            = default;
};

template <typename ValueType, typename NameType>
sqlite3::context_t& serialize(const excluded_t<ValueType, NameType>& t, sqlite3::context_t& context) {
  context << "excluded." << name_of<NameType>::template char_ptr<sqlite3::context_t>();
  return context;
}

template <typename NamedExpr>
auto excluded(const NamedExpr& /*unused*/) -> excluded_t<value_type_of<NamedExpr>, NamedExpr> {
  static_assert(is_selectable_t<NamedExpr>::value, "not a named expression");
  return {};
}

template <typename ValueType, typename AliasProvider>
auto excluded(const ValueType& /*unused*/, const AliasProvider& /*unused*/)
    -> excluded_t<wrap_operand_t<ValueType>, AliasProvider> {
  static_assert(is_value_type_t<ValueType>::value, "first argument is not a value type");
  static_assert(is_alias_provider_t<AliasProvider>::value, "second argument is not an alias provider");
  return {};
}
}  // namespace sqlpp::sqlite3