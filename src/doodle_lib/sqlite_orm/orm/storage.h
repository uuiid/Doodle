#pragma once
#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>

#include <memory>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <type_traits>
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

// 指向成员变量的指针，包含类型信息
struct class_member_ptr {
  std::type_index class_type_{typeid(void)};
  std::type_index class_member_type_{typeid(void)};

  class_member_ptr() = default;

  template <typename T>
  explicit class_member_ptr(T in_ptr) {
    using member_type  = member_type_t<T>;
    using class_type   = member_class_type_t<T>;
    class_type_        = std::type_index(typeid(class_type));
    class_member_type_ = std::type_index(typeid(T));
  }

  ~class_member_ptr() = default;
};

struct column_info {
  std::string name_;
  column_type type_;
  bool not_null_{false};
  bool primary_key_{};
  bool autoincrement_{};
  class_member_ptr ptr_{};  // 指向类成员变量的指针
};

struct foreign_key_info {
  std::string name_;
  class_member_ptr ptr_{};      // 指向类成员变量的指针
  class_member_ptr ref_ptr_{};  // 指向引用表的成员变量的指针
  foreign_key_action on_delete_{foreign_key_action::no_action};
  foreign_key_action on_update_{foreign_key_action::no_action};
};

struct index_info {
  std::string name_;
  class_member_ptr ptr_{};  // 指向类成员变量的指针
};

struct unique_index_info {
  std::string name_;
  std::vector<class_member_ptr> ptrs_;  // 指向类成员变量的指针
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
    l_column.ptr_  = class_member_ptr{in_ptr};
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
    l_fk.ptr_     = class_member_ptr{in_ptr};
    l_fk.ref_ptr_ = class_member_ptr{in_ref_ptr};
    // 解析 in_options
    (([&]() {
       if constexpr (std::is_same_v<decltype(in_options), decltype(on_delete(foreign_key_action::cascade))>) {
         l_fk.on_delete_ = in_options.action_;
       } else if constexpr (std::is_same_v<decltype(in_options), decltype(on_update(foreign_key_action::cascade))>) {
         l_fk.on_update_ = in_options.action_;
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
  std::type_index table_index_{typeid(void)};
  std::pair<std::type_index, std::type_index> left_right_ptrs_{typeid(void), typeid(void)};  // 指向类成员变量的指针
};
template <typename T>
struct where_info {
  std::type_index table_index_{typeid(void)};
  void* ptr_{};  // 指向类成员变量的指针
  where_op op_;
  T value_;
};

struct select_info {
  std::vector<std::type_index> column_ptrs_;           // 指向类成员变量的指针
  std::vector<std::type_index> select_table_indices_;  // 选择的表的类型索引
  std::type_index from_table_index_{typeid(void)};
  std::vector<joint_table_info> joins_;
  template <typename Table>
  select_info& from() {
    from_table_index_ = std::type_index(typeid(Table));
    return *this;
  }

  template <typename Table>
  select_info& join(auto&& LeftPtr, auto&& RightPtr) {
    joint_table_info l_join;
    l_join.table_index_     = std::type_index(typeid(Table));
    l_join.left_right_ptrs_ = {std::type_index(typeid(LeftPtr)), std::type_index(typeid(RightPtr))};
    joins_.push_back(std::move(l_join));
    return *this;
  }

  // 其他成员函数，如 where、order_by 等
};

struct object_t {
  std::type_index table_index_{typeid(void)};
  bool is_order{false};
};
template <typename T>
object_t object(bool is_order = false) {
  return object_t{typeid(T), is_order};
}

template <typename... Args>
select_info select(Args... in_args) {
  // 解析 in_args，构建 select_info,  arg 可能是 object<T>，也可能是 auto T::* 指针
  select_info l_info;
  (([&]() {
     if constexpr (std::is_same_v<std::decay_t<Args>, object_t>) {
       l_info.select_table_indices_.push_back(in_args.table_index_);
     } else if constexpr (std::is_member_object_pointer_v<Args>) {
       l_info.column_ptrs_.push_back(std::type_index(typeid(Args)));
     }
   }()),
   ...);
  return l_info;
}

}  // namespace orm

class storage {
  std::vector<orm::table_info> tables_;
  std::vector<orm::index_info> indexes_;
  std::vector<orm::unique_index_info> unique_indexes_;

 public:
  virtual ~storage() = default;
  storage& reg_table(orm::table_info&& in_table) {
    tables_.push_back(std::move(in_table));
    return *this;
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
      std::vector<orm::class_member_ptr> ptrs{orm::class_member_ptr{in_ptrs}...};
      unique_indexes_.emplace_back(in_index_name, std::move(ptrs));
    } else {
      throw std::runtime_error("Table not found for type " + std::string(typeid(T).name()));
    }
  }
  // 编译 sql
  template <typename T>
  auto& prepare(T&& in_query) {
    return *this;
  }
};
}  // namespace doodle