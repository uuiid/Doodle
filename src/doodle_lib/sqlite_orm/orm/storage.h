#pragma once
#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>

#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

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

struct column_info {
  std::string name_;
  column_type type_;
  bool not_null_{false};
  bool primary_key_{};
  bool autoincrement_{};
  void* ptr_{};  // 指向类成员变量的指针
};

struct foreign_key_info {
  std::string name_;
  std::type_index table_index_{typeid(void)};     // 指向外键类型的指针
  void* ptr_{};                                   // 指向类成员变量的指针
  std::type_index ref_type_index_{typeid(void)};  // 引用表的 type_index
  void* ref_ptr_{};                               // 指向引用表的成员变量的指针
  foreign_key_action on_delete_{foreign_key_action::no_action};
  foreign_key_action on_update_{foreign_key_action::no_action};
};

struct index_info {
  std::string name_;
  void* ptr_{};  // 指向类成员变量的指针
};

struct unique_index_info {
  std::string name_;
  std::vector<void*> ptrs_;  // 指向类成员变量的指针
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

struct table_info {
  std::string name_;
  std::type_index type_index_{typeid(void)};
  std::vector<column_info> columns_;

  template <typename T>
  table_info& add_column(std::string&& in_name, auto T::* in_ptr, auto... in_options) {
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
    columns_.push_back(std::move(l_column));
    return *this;
  }
  template <typename T, typename T2>
  table_info& add_foreign_key(auto T::* in_ptr, auto T2::* in_ref_ptr, auto... in_options) {
    foreign_key_info l_fk;
    l_fk.table_index_    = std::type_index(typeid(T));
    l_fk.ptr_            = reinterpret_cast<void*>(in_ptr);
    l_fk.ref_type_index_ = std::type_index(typeid(T2));
    l_fk.ref_ptr_        = reinterpret_cast<void*>(in_ref_ptr);
    // 解析 in_options
    (([&]() {
       if constexpr (std::is_same_v<decltype(in_options), decltype(on_delete(foreign_key_action::cascade))>) {
         l_fk.on_delete_ = on_delete(foreign_key_action::cascade).action_;
       } else if constexpr (std::is_same_v<decltype(in_options), decltype(on_update(foreign_key_action::cascade))>) {
         l_fk.on_update_ = on_update(foreign_key_action::cascade).action_;
       }
     }()),
     ...);
    return *this;
  }
};

template <typename T>
auto make_table_info(std::string&& in_name) {
  table_info l_table{std::move(in_name), std::type_index(typeid(T))};
  return l_table;
}

struct joint_table_info {
  std::type_index left_table_index_{typeid(void)};
  std::type_index right_table_index_{typeid(void)};
  std::pair<void*, void*> left_right_ptrs_{};  // 指向类成员变量的指针
};
template <typename T>
struct where_info {
  std::type_index table_index_{typeid(void)};
  void* ptr_{};  // 指向类成员变量的指针
  where_op op_;
  T value_;
};

struct select_info {
  std::vector<void*> column_ptrs_;  // 指向类成员变量的指针
  std::type_index from_table_index_{typeid(void)};
  std::vector<joint_table_info> joins_;
};

}  // namespace orm

class storage {
  std::vector<orm::table_info> tables_;
  std::vector<orm::index_info> indexes_;
  std::vector<orm::unique_index_info> unique_indexes_;

 public:
  virtual ~storage() = default;
  orm::table_info o& reg_table(orm::table_info&& in_table) {
    tables_.push_back(std::move(in_table));
    return tables_.back();
  }

  template <typename T>
  void reg_index(const std::string& in_index_name, auto T::* in_ptr) {
    // 查找对应的 table_info
    auto it = std::find_if(tables_.begin(), tables_.end(), [](const orm::table_info& t) {
      return t.type_index_ == std::type_index(typeid(T));
    });
    if (it != tables_.end()) {
      indexes_.emplace_back(in_index_name, in_ptr);
    } else {
      throw std::runtime_error("Table not found for type " + std::string(typeid(T).name()));
    }
  }

  template <typename T>
  void reg_unique_index(const std::string& in_index_name, auto... in_ptrs) {
    // 查找对应的 table_info
    auto it = std::find_if(tables_.begin(), tables_.end(), [](const orm::table_info& t) {
      return t.type_index_ == std::type_index(typeid(T));
    });
    if (it != tables_.end()) {
      std::vector<void*> ptrs{reinterpret_cast<void*>(in_ptrs)...};
      unique_indexes_.emplace_back(in_index_name, std::move(ptrs));
    } else {
      throw std::runtime_error("Table not found for type " + std::string(typeid(T).name()));
    }
  }
};
}  // namespace doodle