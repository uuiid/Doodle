#pragma once
#include "doodle_core/doodle_core_fwd.h"

#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>

#include <atomic>
#include <map>
#include <memory>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <variant>
#include <vector>

namespace doodle {

using storage_column_types =
    std::tuple<std::int64_t, std::double_t, std::string, uuid, chrono::system_zoned_time, nlohmann::json, FSys::path>;

namespace orm {
class storage;
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

template <typename T>
struct column_info {
  using column_ptr_type = name_and_type_ptr<T>::column_type;

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
  struct table_and_column {
    std::string table_;
    std::string column_;
  };
  std::string name_;
  std::vector<table_and_column> ptrs_;
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
  std::vector<std::function<void(storage&)>> to_register_;
  std::vector<foreign_key_info> foreign_keys_;
};

template <typename T>
struct table_info : table_info_base {
  std::vector<column_info<T>> columns_;
  using column_ptr_type = typename column_info<T>::column_ptr_type;

  column_info<T>& find_column_info(auto T::* in_ptr) {
    auto l_iter = std::find_if(columns_.begin(), columns_.end(), [in_ptr](const column_info<T>& in_column) {
      return in_column.ptr_.ptr_ == column_ptr_type{in_ptr};
    });
    if (l_iter == columns_.end()) {
      throw std::runtime_error("Column not found for the given member pointer");
    }
    return *l_iter;
  }

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

  table_info& add_index(std::string&& in_name, auto T::* in_ptr);
  table_info& add_unique_index(std::string&& in_name, auto... in_ptrs);
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

  template <typename T>
  friend struct table_info;
  std::atomic_bool finalized_{false};
 public:
  virtual ~storage() = default;
  template <typename T>
  table_info<T>& reg_table(std::string&& in_name) {
    auto l_table                               = std::make_shared<table_info<T>>();
    l_table->name_                             = std::move(in_name);
    l_table->type_index_                       = std::type_index(typeid(T));
    type_to_table_index_[l_table->type_index_] = tables_.size();
    tables_.push_back(std::move(l_table));
    return static_cast<table_info<T>&>(*tables_.back());
  }

  storage& finalize();


 private:
  template <typename T, typename T2>
  void reg_foreign_key(
      std::string&& in_name, auto T::* in_ptr, auto T2::* in_ref_ptr, foreign_key_action on_delete,
      foreign_key_action on_update
  ) {
    auto l_self_table_index = type_to_table_index_[std::type_index(typeid(T))];
    auto l_ref_table_index  = type_to_table_index_[std::type_index(typeid(T2))];
    auto& l_self_table      = static_cast<table_info<T>&>(*tables_[l_self_table_index]);
    auto& l_ref_table       = static_cast<table_info<T2>&>(*tables_[l_ref_table_index]);
    foreign_key_info l_fk{};
    l_fk.name_      = std::move(in_name);
    l_fk.ptr_       = l_self_table.find_column_info(in_ptr).ptr_.name_;
    l_fk.ref_table_ = l_ref_table.name_;
    l_fk.ref_ptr_   = l_ref_table.find_column_info(in_ref_ptr).ptr_.name_;
    l_fk.on_delete_ = on_delete;
    l_fk.on_update_ = on_update;
    l_self_table.foreign_keys_.push_back(std::move(l_fk));
  }
  template <typename T>
  storage& reg_index(std::string&& in_name, auto T::* in_ptr) {
    auto l_table_index = type_to_table_index_[std::type_index(typeid(T))];
    auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
    index_info l_index{};
    l_index.name_ = std::move(in_name);
    l_index.ptr_  = l_table.find_column_info(in_ptr).ptr_.name_;
    indexes_.push_back(std::move(l_index));
    return *this;
  }
  template <typename T>
  storage& reg_unique_index(std::string&& in_name, auto... in_ptrs) {
    auto l_table_index = type_to_table_index_[std::type_index(typeid(T))];
    auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
    unique_index_info l_unique_index{};
    l_unique_index.name_ = std::move(in_name);
    ((l_unique_index.ptrs_.push_back(
         unique_index_info::table_and_column{l_table.name_, l_table.find_column_info(in_ptrs).ptr_.name_}
     )),
     ...);
    unique_indexes_.push_back(std::move(l_unique_index));
    return *this;
  }
};

template <typename T>
template <typename RefTable>
table_info<T>& table_info<T>::add_foreign_key(
    std::string&& in_name, auto T::* in_ptr, auto RefTable::* in_ref_ptr, foreign_key_action on_delete,
    foreign_key_action on_update
) {
  to_register_.push_back([name_ = std::move(in_name), in_ptr, in_ref_ptr, on_delete, on_update](storage& s) mutable {
    s.reg_foreign_key<T, RefTable>(std::move(name_), in_ptr, in_ref_ptr, on_delete, on_update);
  });
  return *this;
}
template <typename T>
table_info<T>& table_info<T>::add_index(std::string&& in_name, auto T::* in_ptr) {
  to_register_.push_back([name_ = std::move(in_name), in_ptr](storage& s) mutable {
    s.reg_index<T>(std::move(name_), in_ptr);
  });
  return *this;
}
template <typename T>
table_info<T>& table_info<T>::add_unique_index(std::string&& in_name, auto... in_ptrs) {
  to_register_.push_back([name_ = std::move(in_name), in_ptrs...](storage& s) mutable {
    s.reg_unique_index<T>(std::move(name_), in_ptrs...);
  });
  return *this;
}

}  // namespace orm

}  // namespace doodle
