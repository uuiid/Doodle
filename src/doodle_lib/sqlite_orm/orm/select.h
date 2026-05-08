#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <functional>
#include <string>
#include <vector>

namespace doodle::orm {

template <typename Table>
struct object_t {
  using table_type = Table;

  operator std::type_index() const { return std::type_index{typeid(Table)}; }
};

template <typename... TableColumns>
struct select_t {
  static_assert(sizeof...(TableColumns) > 0, "至少需要选择一个列");
  // 结果类型
  using result_type = std::tuple<std::decay_t<TableColumns>...>;
  std::function<std::vector<std::string>(const storage&)> get_column_names_fun_;
};
template <typename Table>
auto object() {
  return object_t<Table>{};
}

namespace detail {

template <typename T>
struct select_arg_type {
  using type = std::decay_t<T>;
};

template <typename C, typename T>
struct select_arg_type<T C::*> {
  using type = std::decay_t<T>;
};

template <typename Table>
struct select_arg_type<object_t<Table>> {
  using type = Table;
};

template <typename T>
using select_arg_type_t = typename select_arg_type<std::decay_t<T>>::type;

}  // namespace detail

// 将select(&Table::column1, &Table::column2, object<Table2>()) 转换为
// select_t<uuid, std::string, Table2>
template <typename... TableColumns>
auto select(TableColumns... in_columns) {
  select_t<detail::select_arg_type_t<TableColumns>...> l_ret{};
  l_ret.get_column_names_fun_ = [columns = std::make_tuple(in_columns...)](const storage& s) {
    std::vector<std::string> column_names;
    std::apply(
        [&s, &column_names](auto&& column) {
          // 处理每个参数
          // 如果是成员指针，获取列名
          if constexpr (std::is_member_pointer_v<std::decay_t<decltype(column)>>) {
            column_names.push_back(s.get_column_name(column));
          } else if constexpr (std::is_same_v<
                                   std::decay_t<decltype(column)>,
                                   object_t<detail::select_arg_type_t<decltype(column)>>>) {
            // 如果是object<Table>，获取表的所有列名
            using table_type        = detail::select_arg_type_t<decltype(column)>;
            auto table_column_names = s.get_table_column_names<table_type>();
            column_names.insert(column_names.end(), table_column_names.begin(), table_column_names.end());
          } else {
            static_assert(always_false<decltype(column)>, "不支持的参数类型");
          }
        },
        columns
    );
    return column_names;
  };
  return l_ret;
}

}  // namespace doodle::orm