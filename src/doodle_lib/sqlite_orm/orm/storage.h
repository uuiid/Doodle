#pragma once
#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>

#include <sqlite_orm/sqlite_orm.h>
#include <typeindex>
#include <utility>

namespace doodle {

namespace orm {
// 使用 boost::pfr 来获取结构体成员变量的指针
template <typename T, std::size_t Index>
auto get_member_ptr() {

  
}

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
  bool not_null_{};
  bool primary_key_{};
  bool autoincrement_{};
  void* ptr_{};  // 指向类成员变量的指针
};

struct table_info {
  std::string name_;
  std::vector<column_info> columns_;
  std::type_index type_index_;
};
template <typename T>
auto make_table_info() {
  boost::pfr::for_each_field(std::declval<T>(), [](const auto& field, std::size_t index) {
    using field_type  = std::decay_t<decltype(field)>;
    // 获取成员变量的指针
    auto ptr          = boost::pfr::get_relative_address<T, index, T>();
    // 获取成员变量的类型
    using member_type = std::remove_reference_t<decltype(std::declval<T>().*ptr)>;
    // 获取成员变量的名称
    std::string name  = boost::pfr::get_name<index, T>();
    // 获取成员变量的类型
    column_type type;
    if constexpr (std::is_integral_v<member_type>) {
      type = column_type::integer;
    } else if constexpr (std::is_floating_point_v<member_type>) {
      type = column_type::real;
    } else if constexpr (std::is_same_v<member_type, std::string>) {
      type = column_type::text;
    } else {
      type = column_type::blob;
    }
  });
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