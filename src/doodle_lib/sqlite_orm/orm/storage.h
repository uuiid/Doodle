#pragma once
#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>

#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <typeindex>
#include <utility>

namespace doodle {

namespace orm {
// 使用 boost::pfr 来获取结构体成员变量的指针
template <typename T, std::size_t Index>
auto get_member_ptr() {}

enum class column_type {
  null,
  integer,
  real,
  text,
  blob,
};

struct column_info {
  std::string name_;
  column_type type_;
  bool not_null_{false};
  bool primary_key_{};
  bool autoincrement_{};
  void* ptr_{};  // 指向类成员变量的指针
};

struct table_info {
  std::string name_;
  std::type_index type_index_{typeid(void)};
  std::vector<column_info> columns_;

  table_info& add_column(column_info&& in_column) {
    columns_.push_back(std::move(in_column));
    return *this;
  }
};

struct not_null {};
struct primary_key {};
struct autoincrement {};

template <typename T>
auto make_column(std::string&& in_name, auto T::* in_ptr, auto... in_options) {
  column_info l_column;
  l_column.name_ = std::move(in_name);
  l_column.ptr_  = reinterpret_cast<void*>(in_ptr);
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
  return l_column;
}

template <typename T>
auto make_table_info(std::string&& in_name) {
  table_info l_table{std::move(in_name), std::type_index(typeid(T))};
  return l_table;
}
}  // namespace orm

class storage {
  std::vector<orm::table_info> tables_;

 public:
  virtual ~storage() = default;
  storage& reg(orm::table_info&& in_table) {
    tables_.push_back(std::move(in_table));
    return *this;
  }
};
}  // namespace doodle