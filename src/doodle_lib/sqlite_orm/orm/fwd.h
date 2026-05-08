#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle::orm {
using storage_column_types =
    std::tuple<std::int64_t, std::double_t, std::string, uuid, chrono::system_zoned_time, nlohmann::json, FSys::path>;
class storage;

template <typename Table, typename Tuple>
struct tuple_to_table_member_variant;
template <typename Table, typename... Ts>
struct tuple_to_table_member_variant<Table, std::tuple<Ts...>> {
  using type = std::variant<Ts Table::*...>;
};

template <typename Table>
using table_columns_t = typename tuple_to_table_member_variant<Table, storage_column_types>::type;

template <typename T>
struct name_and_type_ptr {
  std::string name_;
  table_columns_t<T> ptr_;

  using column_type = table_columns_t<T>;
};

enum class column_type {
  null,
  integer,
  real,
  text,
  blob,
};

enum class foreign_key_action {
  no_action,
  restrict,
  set_null,
  set_default,
  cascade,
};

enum class where_op {
  equal,
  not_equal,
  greater,
  less,
  greater_equal,
  less_equal,
  like,
  in,
};

enum class join_type {
  inner,
  left,
  right,
  full,
};

template <typename T>
struct member_type;  // 主模板：不定义，仅用于特化

// 特化：数据成员指针 (T C::*)
template <typename C, typename T>
struct member_type<T C::*> {
  using ptr_type   = T;
  using class_type = C;
};
// 辅助别名
template <typename T>
using member_type_t = typename member_type<T>::ptr_type;
template <typename T>
using member_class_type_t = typename member_type<T>::class_type;

}  // namespace doodle::orm