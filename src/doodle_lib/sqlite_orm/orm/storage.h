#pragma once
#include "doodle_core/doodle_core_fwd.h"

#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>

#include <map>
#include <memory>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>

namespace doodle {
class storage;

using storage_column_types =
    std::tuple<std::int64_t, std::double_t, std::string, uuid, chrono::system_zoned_time, nlohmann::json>;

namespace orm {
struct object_t;
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
  std::string_view name_;
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

template <typename T>
struct column_info {
  name_and_type_ptr<T> ptr_{};
  bool not_null_{false};
  bool primary_key_{};
  bool autoincrement_{};
  column_type type_{column_type::null};
};

struct foreign_key_info {
  std::string name_;
  std::string ptr_{};
  std::string ref_table_;
  std::string ref_ptr_{};
  foreign_key_action on_delete_{foreign_key_action::no_action};
  foreign_key_action on_update_{foreign_key_action::no_action};
};

struct index_info {
  std::string name_;
  std::string ptr_{};
};

struct unique_index_info {
  std::string name_;
  std::vector<std::string> ptrs_;
};

struct not_null {};
struct primary_key {};
struct autoincrement {};

struct on_delete {
  foreign_key_action action_;
  explicit on_delete(foreign_key_action action) : action_(action) {}
};
struct on_update {
  foreign_key_action action_;
  explicit on_update(foreign_key_action action) : action_(action) {}
};

struct table_info_base {
  std::string name_;
  std::type_index type_index_{typeid(void)};
  std::vector<std::function<void(storage&)>> foreign_keys_to_register_;
};

template <typename T>
struct table_info : table_info_base {
  std::vector<column_info<T>> columns_;

  table_info& add_column(std::string&& in_name, auto T::* in_ptr, auto... in_options) {
    column_info<T> l_column;
    l_column.ptr_.name_ = std::move(in_name);
    l_column.ptr_.ptr_  = in_ptr;
    // 解析 in_options
    (([&]() {
       if constexpr (std::is_same_v<decltype(in_options), decltype(not_null())>) {
         l_column.not_null_ = true;
       } else if constexpr (std::is_same_v<decltype(in_options), decltype(primary_key())>) {
         l_column.primary_key_ = true;
       } else if constexpr (std::is_same_v<decltype(in_options), decltype(autoincrement())>) {
         l_column.autoincrement_ = true;
       }
     }()),
     ...);
    // 根据成员变量类型推断 column_type
    using member_type = std::remove_reference_t<std::remove_pointer_t<decltype(in_ptr)>>;
    if constexpr (std::is_integral_v<member_type>) {
      l_column.type_ = column_type::integer;
    } else if constexpr (std::is_floating_point_v<member_type>) {
      l_column.type_ = column_type::real;
    } else if constexpr (std::is_same_v<member_type, std::string>) {
      l_column.type_ = column_type::text;
    } else {
      l_column.type_ = column_type::blob;
    }
    columns_.push_back(std::move(l_column));
    return *this;
  }
  template <typename RefTable>
  table_info& add_foreign_key(
      std::string&& in_name, auto T::* in_ptr, auto RefTable::* in_ref_ptr,
      foreign_key_action on_delete = foreign_key_action::no_action,
      foreign_key_action on_update = foreign_key_action::no_action
  );
};

template <typename T>
auto make_table_info(std::string&& in_name) {
  table_info<T> l_table{std::move(in_name), std::type_index(typeid(T))};
  return l_table;
}
class storage {
  std::vector<std::shared_ptr<table_info_base>> tables_;
  std::vector<index_info> indexes_;
  std::vector<unique_index_info> unique_indexes_;

  std::map<std::type_index, std::size_t> type_to_table_index_;

 public:
  virtual ~storage() = default;
  template <typename T>
  table_info<T>& reg_table(std::string&& in_name) {
    auto l_table = std::make_shared<table_info<T>>(make_table_info<T>(std::move(in_name)));
    type_to_table_index_[l_table->type_index_] = tables_.size();
    tables_.push_back(std::move(l_table));
    return static_cast<table_info<T>&>(*tables_.back());
  }
  // 表注册结束, 在 finalize 中构建 type_index 到 table 索引的映射
  storage& finalize();

  template <typename T, typename T2>
  storage& reg_foreign_key(
      std::string&& in_name, auto T::* in_ptr, auto T2::* in_ref_ptr,
      foreign_key_action on_delete = foreign_key_action::no_action,
      foreign_key_action on_update = foreign_key_action::no_action
  );
  template <typename T>
  storage& reg_index(std::string&& in_name, auto T::* in_ptr);
  template <typename T>
  storage& reg_unique_index(std::string&& in_name, auto... in_ptrs);
};

template <typename T>
template <typename RefTable>
table_info<T>& table_info<T>::add_foreign_key(
    std::string&& in_name, auto T::* in_ptr, auto RefTable::* in_ref_ptr, foreign_key_action on_delete,
    foreign_key_action on_update
) {
  foreign_keys_to_register_.push_back([name_ = std::move(in_name), in_ptr, in_ref_ptr, on_delete,
                                       on_update](storage& s) {
    s.reg_foreign_key<T, RefTable>(name_, in_ptr, in_ref_ptr, on_delete, on_update);
  });
  return *this;
}
}  // namespace orm

}  // namespace doodle
