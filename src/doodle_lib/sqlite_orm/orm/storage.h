#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>

#include <atomic>
#include <fmt/format.h>
#include <functional>
#include <map>
#include <memory>
#include <sqlite3.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

namespace doodle {

namespace orm {
struct column_info {
  using column_ptr_type = table_columns_t;

  std::string name_;
  table_columns_t ptr_;
  bool not_null_{false};
  bool primary_key_{};
  bool autoincrement_{};
  bool unique_{};
  column_type type_{column_type::null};
  std::string default_value_{};
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
  std::string name_;
  std::string table_name_;
  std::vector<std::string> ptrs_;
};

struct not_null {};
struct primary_key {};
struct autoincrement {};
struct unindexed {};
struct unique {};
struct default_value {
  std::string value_;
  explicit default_value(std::string value);
};

struct on_delete {
  foreign_key_action action_;
  explicit on_delete(foreign_key_action action);
};
struct on_update {
  foreign_key_action action_;
  explicit on_update(foreign_key_action action);
};

struct table_info_base {
  std::string name_;
  std::type_index type_index_{typeid(void)};
  std::vector<std::function<void(storage&)>> to_register_;
  std::vector<foreign_key_info> foreign_keys_;
  std::vector<column_info> columns_;

  virtual ~table_info_base() = default;
  std::vector<std::string> get_foreign_key_create_sql() const;
  virtual std::string get_table_create_sql() const = 0;

  template <typename T>
  column_info& find_column_info(auto T::* in_ptr);
  column_info& find_column_info(const table_columns_t& in_column);
};

struct table_fts_info : table_info_base {
  std::vector<bool> unindexed_columns_;
  using column_ptr_type = typename column_info::column_ptr_type;

  std::string tokenizer_{};
  std::string content_table_{};
  std::string content_rowid_{};

  table_fts_info& content(const std::string& content_table, const std::string& content_rowid);
  table_fts_info& tokenizer(const std::string& tokenizer);

  template <typename T>
  table_fts_info& add_column(std::string&& in_name, auto T::* in_ptr, auto... in_options);

  std::string get_table_create_sql() const override;
};

struct table_info : table_info_base {
  using column_ptr_type = typename column_info::column_ptr_type;

  template <typename T>
  table_info& add_column(std::string&& in_name, auto T::* in_ptr, auto... in_options);
  template <typename T, typename RefTable>
  table_info& add_foreign_key(
      std::string&& in_name, auto T::* in_ptr, auto RefTable::* in_ref_ptr,
      foreign_key_action on_delete = foreign_key_action::no_action,
      foreign_key_action on_update = foreign_key_action::no_action
  );

  template <typename T>
  table_info& add_index(std::string&& in_name, auto T::* in_ptr);
  template <typename T>
  table_info& add_unique_index(std::string&& in_name, auto T::*... in_ptrs);

  std::string get_table_create_sql() const override;
};

enum class trigger_timing { before, after, instead_of };

enum class trigger_event { insert, update, delete_ };

struct trigger_info {
  std::string name_;
  trigger_timing timing_;             // BEFORE, AFTER, INSTEAD OF
  trigger_event event_;               // INSERT, UPDATE, DELETE
  std::vector<std::string> columns_;  // 仅针对 UPDATE 事件
  std::string table_name_;
  std::string statement_;
};

struct sqlite_stmt {
  sqlite3_stmt* stmt_{nullptr};
  sqlite3* db_{nullptr};
  std::int32_t bind_index_{0};  // sqlite bind index starts from 1, but we use 0-based index internally
 public:
  sqlite_stmt() = default;
  explicit sqlite_stmt(storage& db, const std::string& sql);
  ~sqlite_stmt();
  void prepare(storage& db, const std::string& sql);
  // 重置绑定参数索引，以便重用 sqlite_stmt 对象执行多次 SQL 语句
  void reset_bind();
  std::int32_t get_bind_index();
  std::int64_t get_column_count() const;

  void step();
  std::int32_t step_not_throw();

  template <typename T>
  T get_column_value(int columnIndex) const;
  template <typename T>
  void bind(std::int32_t in_index, T&& in_value);
};

struct table_info_t : public table_info_base_t {
  std::type_index type_index_{typeid(void)};
  template <typename T>
  explicit table_info_t(T&&) : type_index_(typeid(T)) {}
  virtual std::string get_table_name(const storage& s) const override;
};

class storage {
  std::vector<std::shared_ptr<table_info_base>> tables_;
  std::vector<index_info> indexes_;
  std::vector<unique_index_info> unique_indexes_;
  std::map<std::type_index, std::size_t> type_to_table_index_;
  std::vector<std::shared_ptr<trigger_info>> triggers_;

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
  explicit storage(FSys::path in_path, std::int32_t in_flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
  ~storage();

  void open(FSys::path in_path, std::int32_t in_flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
  void open();

  void sync_schema();

  template <typename T>
  table_info& reg_table(std::string&& in_name);
  template <typename T>
  table_info& reg_virtual_table(std::string&& in_name);
  create_trigger_t create_trigger(std::string in_name);

  storage& finalize();

  template <typename T>
  bool has_reg_table();

  template <typename T>
  std::string get_column_name(auto T::* in_ptr, bool add_table_name = true) const;
  std::string get_column_name(const table_columns_t& in_column, bool add_table_name = true) const;
  template <typename T>
  std::vector<std::string> get_table_column_names() const;
  template <typename T>
  const std::vector<column_info>& get_table_columns() const;
  template <typename T>
  std::string get_table_name() const;
  std::string get_table_name(std::type_index in_type_index) const;

  // 事务相关
  void begin_transaction();
  void commit_transaction();
  void rollback_transaction();

  struct transaction_guard {
    storage& s_;
    bool committed_{false};
    explicit transaction_guard(storage& s);
    void commit();
    void rollback();
    ~transaction_guard();
  };

  transaction_guard transaction();

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
};

}  // namespace orm

}  // namespace doodle

namespace fmt {
template <>
struct formatter<doodle::orm::trigger_timing> : formatter<std::string_view> {
  static std::string_view to_string(doodle::orm::trigger_timing timing) {
    switch (timing) {
      case doodle::orm::trigger_timing::before:
        return "BEFORE";
      case doodle::orm::trigger_timing::after:
        return "AFTER";
      case doodle::orm::trigger_timing::instead_of:
        return "INSTEAD OF";
      default:
        throw std::runtime_error("Invalid trigger timing");
    }
  }

  template <typename FormatContext>
  auto format(doodle::orm::trigger_timing timing, FormatContext& ctx) const {
    return formatter<std::string_view>::format(to_string(timing), ctx);
  }
};
template <>
struct formatter<doodle::orm::trigger_event> : formatter<std::string_view> {
  static std::string_view to_string(doodle::orm::trigger_event event) {
    switch (event) {
      case doodle::orm::trigger_event::insert:
        return "INSERT";
      case doodle::orm::trigger_event::update:
        return "UPDATE OF";
      case doodle::orm::trigger_event::delete_:
        return "DELETE";
      default:
        throw std::runtime_error("Invalid trigger event");
    }
  }

  template <typename FormatContext>
  auto format(doodle::orm::trigger_event event, FormatContext& ctx) const {
    return formatter<std::string_view>::format(to_string(event), ctx);
  }
};
}  // namespace fmt
