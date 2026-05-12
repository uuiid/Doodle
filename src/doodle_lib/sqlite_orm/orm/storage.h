#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <variant>
#include <vector>

namespace doodle {

namespace orm {

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
  std::string table_name_;
  std::string column_name_;
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

  virtual ~table_info_base()                                     = default;
  virtual std::vector<std::string> get_column_create_sql() const = 0;
  std::vector<std::string> get_foreign_key_create_sql() const {
    std::vector<std::string> l_sqls;
    for (const auto& fk : foreign_keys_) {
      std::string l_sql = fmt::format(
          "FOREIGN KEY({}) REFERENCES {}({}) ON DELETE {} ON UPDATE {}", fk.ptr_, fk.ref_table_, fk.ref_ptr_,
          fk.on_delete_, fk.on_update_
      );
      l_sqls.push_back(std::move(l_sql));
    }
    return l_sqls;
  }
};

template <typename T>
struct table_info : table_info_base {
  std::vector<column_info<T>> columns_;
  using column_ptr_type = typename column_info<T>::column_ptr_type;

  column_info<T>& find_column_info(auto T::* in_ptr) {
    auto l_iter = std::find_if(columns_.begin(), columns_.end(), [in_ptr](const column_info<T>& in_column) {
      return in_column.ptr_.ptr_ == column_ptr_type{in_ptr};
    });
    if (l_iter == columns_.end()) throw std::runtime_error("Column not found for the given member pointer");

    return *l_iter;
  }
  column_info<T>& find_column_info(const table_columns_t<T>& in_column) {
    auto l_iter = std::find_if(columns_.begin(), columns_.end(), [&in_column](const column_info<T>& in_col) {
      return in_col.ptr_.ptr_ == in_column;
    });
    if (l_iter == columns_.end()) throw std::runtime_error("Column not found for the given column pointer");

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
    using member_type = std::remove_reference_t<std::remove_pointer_t<member_type_t<decltype(in_ptr)>>>;
    l_column.type_    = sqlite_statement_printer<member_type>{}();
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

  std::vector<std::string> get_column_create_sql() const override {
    std::vector<std::string> l_column_sqls;
    for (const auto& column : columns_) {
      std::string l_sql = fmt::format("{} {}", column.ptr_.name_, column.type_);
      if (column.primary_key_) {
        l_sql += " PRIMARY KEY";
      }
      if (column.autoincrement_) {
        l_sql += " AUTOINCREMENT";
      }
      if (column.not_null_) {
        l_sql += " NOT NULL";
      }
      l_column_sqls.push_back(std::move(l_sql));
    }
    return l_column_sqls;
  }
};

struct sqlite_stmt {
  sqlite3_stmt* stmt_{nullptr};
  sqlite3* db_{nullptr};
  std::int32_t bind_index_{0};  // sqlite bind index starts from 1, but we use 0-based index internally
 public:
  sqlite_stmt() = default;
  explicit sqlite_stmt(storage& db, const std::string& sql) { prepare(db, sql); }
  ~sqlite_stmt();
  void prepare(storage& db, const std::string& sql);
  std::int32_t get_bind_index();
  std::int64_t get_column_count() const;

  std::int32_t bind(const storage_column_variant& value);
  void step();
  std::int32_t step_not_throw();

  template <typename T>
  T get_column_value(int columnIndex) const;
};

class storage {
  std::vector<std::shared_ptr<table_info_base>> tables_;
  std::vector<index_info> indexes_;
  std::vector<unique_index_info> unique_indexes_;

  std::map<std::type_index, std::size_t> type_to_table_index_;

  template <typename T>
  friend struct table_info;

  friend struct sqlite_stmt;
  friend struct select_t;
  friend struct insert_t;
  friend struct update_t;

  std::atomic_bool finalized_{false};
  FSys::path db_path_;

  sqlite3* db_{nullptr};

 public:
  storage() = default;
  explicit storage(FSys::path in_path, std::int32_t in_flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) {
    open(std::move(in_path), in_flags);
  }
  ~storage();

  void open(FSys::path in_path, std::int32_t in_flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
  void open() { open({}); }

  void sync_schema();

  template <typename T>
  table_info<T>& reg_table(std::string&& in_name);

  storage& finalize();

  template <typename T>
  bool has_reg_table();

  template <typename T>
  std::string get_column_name(auto T::* in_ptr, bool add_table_name = true) const;
  template <typename T>
  std::string get_column_name(const table_columns_t<T>& in_column, bool add_table_name = true) const;
  template <typename T>
  std::vector<std::string> get_table_column_names() const;
  template <typename T>
  const std::vector<column_info<T>>& get_table_columns() const;

 private:
  template <typename T, typename T2>
  void reg_foreign_key(
      std::string&& in_name, auto T::* in_ptr, auto T2::* in_ref_ptr, foreign_key_action on_delete,
      foreign_key_action on_update
  );
  template <typename T>
  void reg_index(std::string&& in_name, auto T::* in_ptr);
  template <typename T>
  void reg_unique_index(std::string&& in_name, auto... in_ptrs);

  std::string get_table_name(std::type_index in_type_index) const;
  template <typename T>
  std::string get_table_name() const {
    return get_table_name(std::type_index(typeid(T)));
  }
};

struct where_info_t {
  std::function<std::string(const storage&)> condition_fun_{};
  std::function<void(sqlite_stmt&)> bind_fun_{};

  std::string operator()(const storage& s) const {
    if (condition_fun_) return condition_fun_(s);
    return "";
  }
  void bind(sqlite_stmt& stmt) const {
    if (bind_fun_) bind_fun_(stmt);
  }

  operator bool() const { return static_cast<bool>(condition_fun_); }
};

}  // namespace orm

}  // namespace doodle
