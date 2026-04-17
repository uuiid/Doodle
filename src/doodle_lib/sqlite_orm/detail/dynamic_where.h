#pragma once

#include <array>
#include <functional>
#include <memory>
#include <sqlite_orm/sqlite_orm.h>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace sqlite_orm {

namespace internal {

/**
 * C - serializer context class
 */
template <class C>
struct dynamic_where_t {
  using context_t = C;

  // Holds type-erased expression: lazy serialization + AST traversal for binding.
  struct entry_t {
    std::function<std::string(const context_t&)> serialize_fn;
    std::function<void(conditional_binder&)> bind_fn;
  };

  using const_iterator = typename std::vector<entry_t>::const_iterator;

  dynamic_where_t(context_t) {}

  template <class E>
  void push_back(E&& expression) {
    // Share the expression between the two closures without double-move.
    auto shared_expr = std::make_shared<std::decay_t<E>>(std::forward<E>(expression));
    entry_t entry;
    entry.serialize_fn = [shared_expr](const context_t& in_context) { return serialize(*shared_expr, in_context); };
    entry.bind_fn      = [shared_expr](conditional_binder& binder) { iterate_ast(*shared_expr, binder); };
    this->entries.push_back(std::move(entry));
  }

  const_iterator begin() const { return this->entries.begin(); }

  const_iterator end() const { return this->entries.end(); }

  bool empty() const { return this->entries.empty(); }

  void clear() { this->entries.clear(); }

  std::size_t size() const { return this->entries.size(); }

 protected:
  std::vector<entry_t> entries;
};

template <class T>
inline constexpr bool is_dynamic_where_v = polyfill::is_specialization_of<T, dynamic_where_t>::value;

template <class T>
struct is_dynamic_where : polyfill::bool_constant<is_dynamic_where_v<T>> {};

template <class T>
inline constexpr bool is_where_clause_v = polyfill::disjunction<is_where<T>, is_dynamic_where<T>>::value;

template <class T>
struct is_where_clause : polyfill::bool_constant<is_where_clause_v<T>> {};

// AST traversal support: allows conditional_binder to reach stored expressions.
template <class C>
struct ast_iterator<dynamic_where_t<C>, void> {
  using node_type = dynamic_where_t<C>;

  template <class L>
  void operator()(const node_type& where, L& lambda) const {
    // Only handle conditional_binder; other visitors see no nodes (safe no-op).
    if constexpr (std::is_same_v<polyfill::remove_cvref_t<L>, conditional_binder>) {
      for (const auto& entry : where) {
        entry.bind_fn(lambda);
      }
    }
  }
};

template <class C>
struct statement_serializer<dynamic_where_t<C>, void> {
  using statement_type = dynamic_where_t<C>;

  template <class Ctx>
  std::string operator()(const statement_type& statement, const Ctx& in_context) const {
    if (statement.empty()) {
      return {};
    }

    std::stringstream ss;

    static constexpr std::array<orm_gsl::czstring, 2> sep = {" AND ", ""};
    auto l_len                                            = statement.size();
    if (l_len == 0) throw std::logic_error("dynamic_where statement cannot be empty");
    for (bool first = true; const typename statement_type::entry_t& entry : statement) {
      ss << sep[std::exchange(first, false)] << (l_len == 1 ? "" : "(") << entry.serialize_fn(in_context)
         << (l_len == 1 ? "" : ")");
    }

    return ss.str();
  }
};

template <class T>
struct content_rowid_t {
  using value_type = T;

  value_type value;
};

template <class T>
struct statement_serializer<content_rowid_t<T>, void> {
  using statement_type = content_rowid_t<T>;

  template <class Ctx>
  SQLITE_ORM_STATIC_CALLOP std::string operator()(
      const statement_type& statement, const Ctx& context
  ) SQLITE_ORM_OR_CONST_CALLOP {
    std::stringstream ss;
    ss << "content_rowid=" << serialize(statement.value, context);
    return ss.str();
  }
};

}  // namespace internal

/**
 * WHERE condition pack.
 * Difference from `where(...)` is that `dynamic_where` can be changed at runtime using `push_back` member.
 */
template <class S>
internal::dynamic_where_t<internal::serializer_context<typename S::db_objects_type>> dynamic_where(const S& storage) {
  return {obtain_db_objects(storage)};
}

template <class T>
internal::content_rowid_t<T> content_rowid(T value) {
  return {std::move(value)};
}

}  // namespace sqlite_orm