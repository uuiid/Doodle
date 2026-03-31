#pragma once

#include <array>
#include <functional>
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
  using context_t      = C;
  using serialize_fun  = std::function<std::string(context_t)>;
  using entry_t        = serialize_fun;
  using const_iterator = typename std::vector<entry_t>::const_iterator;

  dynamic_where_t(context_t) {}

  template <class E>
  void push_back(E expression) {
    // NOTE: follow sqlite_orm dynamic_set/dynamic_order_by pattern:
    // store fully serialized expression, with bindables inlined.
    this->entries.push_back([expression = std::move(expression)](const context_t& in_context) {
      return serialize(expression, in_context);
    });
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
    for (bool first = true; const typename statement_type::entry_t& entry : statement) {
      ss << sep[std::exchange(first, false)] << (l_len == 1 ? "" : "(") << entry(in_context) << (l_len == 1 ? "" : ")");
    }

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

}  // namespace sqlite_orm