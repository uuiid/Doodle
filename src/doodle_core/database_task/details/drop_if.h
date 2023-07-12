//
// Created by td_main on 2023/7/12.
//

#pragma once
#include <sqlpp11/data_types/boolean/data_type.h>
#include <sqlpp11/data_types/integral/data_type.h>
#include <sqlpp11/data_types/text/data_type.h>
#include <sqlpp11/data_types/time_point/data_type.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
namespace sqlpp::sqlite3 {

// 单表名称数据
template <typename Database, typename Table>
struct drop_data_t {
  drop_data_t(Table table) : _table(table) {}

  drop_data_t(const drop_data_t&)            = default;
  drop_data_t(drop_data_t&&)                 = default;
  drop_data_t& operator=(const drop_data_t&) = default;
  drop_data_t& operator=(drop_data_t&&)      = default;
  ~drop_data_t()                             = default;

  Table _table;
};

// 擦除
template <typename Database, typename Table>
struct drop_t {
  using _traits = make_traits<no_value_t>;

  using _data_t = drop_data_t<Database, Table>;

  struct _alias_t {};

  // 有数据和方法的成员执行
  template <typename Policies>
  struct _impl_t {
    // workaround for msvc bug https://connect.microsoft.com/VisualStudio/Feedback/Details/2091069
    _impl_t() = default;
    _impl_t(const _data_t& data) : _data(data) {}

    _data_t _data;
  };

  // Base template to be inherited by the statement
  template <typename Policies>
  struct _base_t {
    using _data_t = drop_data_t<Database, Table>;

    // workaround for msvc bug https://connect.microsoft.com/VisualStudio/Feedback/Details/2091069
    template <typename... Args>
    _base_t(Args&&... args) : drop{std::forward<Args>(args)...} {}

    _impl_t<Policies> drop;
    _impl_t<Policies>& operator()() { return drop; }
    const _impl_t<Policies>& operator()() const { return drop; }

    template <typename T>
    static auto _get_member(T t) -> decltype(t.into) {
      return t.drop;
    }

    using _consistency_check = consistent_t;
  };
};
SQLPP_PORTABLE_STATIC_ASSERT(assert_drop_t, "drop() required");
SQLPP_PORTABLE_STATIC_ASSERT(assert_drop_arg_is_table, "argument for drop() must be a table");
template <typename T>
struct check_drop {
  using type = static_combined_check_t<static_check_t<is_raw_table_t<T>::value, assert_drop_arg_is_table>>;
};
template <typename T>
using check_drop_t = typename check_drop<wrap_operand_t<T>>::type;

// 删除
struct no_drop_t {
  using _traits = make_traits<no_value_t>;
  //  using _nodes = detail::type_vector<>;

  // Data
  using _data_t = no_data_t;

  // Member implementation with data and methods
  template <typename Policies>
  struct _impl_t {
    _data_t _data;
  };

  // Base template to be inherited by the statement
  template <typename Policies>
  struct _base_t {
    using _data_t = no_data_t;

    _impl_t<Policies> no_drop;
    _impl_t<Policies>& operator()() { return no_drop; }
    const _impl_t<Policies>& operator()() const { return no_drop; }

    template <typename T>
    static auto _get_member(T t) -> decltype(t.no_drop) {
      return t.no_drop;
    }

    using _database_t = typename Policies::_database_t;

    template <typename Check, typename T>
    using _new_statement_t   = new_statement_t<Check, Policies, no_drop_t, T>;

    using _consistency_check = assert_drop_t;

    template <typename Table>
    auto drop(Table table) const -> _new_statement_t<check_drop_t<Table>, drop_t<void, Table>> {
      return _drop_impl<void>(check_drop_t<Table>{}, table);
    }

   private:
    template <typename Database, typename Check, typename Table>
    auto _drop_impl(Check, Table table) const -> inconsistent<Check>;

    template <typename Database, typename Table>
    auto _drop_impl(consistent_t /*unused*/, Table table) const
        -> _new_statement_t<consistent_t, drop_t<Database, Table>> {
      static_assert(
          required_tables_of<drop_t<Database, Table>>::size::value == 0, "argument depends on another table in drop()"
      );

      return {static_cast<const derived_statement_t<Policies>&>(*this), drop_data_t<Database, Table>{table}};
    }
  };
};

// Interpreters
template <typename Context, typename Database, typename Table>
Context& serialize(const drop_data_t<Database, Table>& t, Context& context) {
  serialize(t._table, context);
  return context;
}

struct drop_if_exists_name_t {};
struct drop_if_exists_t : public sqlpp::statement_name_t<drop_if_exists_name_t> {
 public:
  using _traits = make_traits<no_value_t, tag::is_return_value>;
  struct _alias_t {};

  template <typename statement_t>
  struct _result_methods_t {
    using _statement_t = statement_t;

    const _statement_t& _get_statement() const { return static_cast<const _statement_t&>(*this); }

    template <typename Db>
    auto _run(Db& db) const -> decltype(db.execute(this->_get_statement())) {
      return db.execute(_get_statement());
    }

    template <typename Db>
    auto _prepare(Db& db) const -> prepared_execute_t<Db, _statement_t> {
      return {{}, db.prepare_execute(_get_statement())};
    }
  };
};

template <typename Context>
Context& serialize(const sqlite3::drop_if_exists_name_t&, Context& context) {
  context << "DROP TABLE IF EXISTS ";
  return context;
}

template <typename database>
using blank_drop_if_exists_table_t = statement_t<database, drop_if_exists_t, no_drop_t>;

template <typename table>
auto drop_if_exists_table(table in_table) -> decltype(blank_drop_if_exists_table_t<void>{}.drop(in_table)) {
  return blank_drop_if_exists_table_t<void>{}.drop(in_table);
}

}  // namespace sqlpp::sqlite3
