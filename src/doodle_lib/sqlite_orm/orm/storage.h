#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <boost/lockfree/lockfree_forward.hpp>
#include <boost/lockfree/policies.hpp>
#include <boost/lockfree/stack.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>

#include <atomic>
#include <chrono>
#include <fmt/format.h>
#include <map>
#include <memory>
#include <sqlite3.h>
#include <string>
#include <string_view>
#include <tbb/concurrent_queue.h>
#include <typeindex>
#include <vector>

namespace doodle {

namespace orm {

enum class journal_mode_t { delete_, truncate, persist, memory, wal, off };

struct column_info {
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
  column_info_ptr ptr_{};
  table_info_base_ptr ref_table_;
  column_info_ptr ref_ptr_{};
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
  std::vector<foreign_key_info> foreign_keys_;
  std::vector<column_info> columns_;
  std::vector<std::shared_ptr<create_index_base_t>> indexes_;

  virtual ~table_info_base() = default;
  std::vector<std::string> get_foreign_key_create_sql(session& s, const to_sql_ctx& ctx) const;
  virtual std::string to_sql(session& s, const to_sql_ctx& ctx) const = 0;

  template <typename T>
  column_info& find_column_info(auto T::* in_ptr);
  column_info& find_column_info(const table_columns_t& in_column);
};

struct table_fts_info : table_info_base {
  std::vector<bool> unindexed_columns_;
  using column_ptr_type = table_columns_t;

  std::string tokenizer_{};
  table_info_base_ptr content_table_{};
  column_info_ptr content_rowid_{};
  template <typename Table>
  table_fts_info& content();
  template <typename Table, typename ValueType>
  table_fts_info& content_id(ValueType Table::* in_rowid_ptr);

  table_fts_info& tokenizer(const std::string& tokenizer);

  template <typename T>
  table_fts_info& add_column(std::string&& in_name, auto T::* in_ptr, auto... in_options);

  std::string to_sql(session& s, const to_sql_ctx& ctx) const override;
};

struct table_info : table_info_base {
  using column_ptr_type = table_columns_t;

  template <typename T>
  table_info& add_column(std::string&& in_name, auto T::* in_ptr, auto... in_options);
  template <typename T, typename RefTable>
  table_info& add_foreign_key(
      auto T::* in_ptr, auto RefTable::* in_ref_ptr, foreign_key_action on_delete = foreign_key_action::no_action,
      foreign_key_action on_update = foreign_key_action::no_action
  );

  template <typename T>
  table_info& add_index(auto T::* in_ptr);
  template <typename T>
  table_info& add_unique_index(auto T::*... in_ptrs);
  table_info& add_index(const create_index_base_t& index);

  std::string to_sql(session& s, const to_sql_ctx& ctx) const override;
};

enum class trigger_timing { before, after, instead_of };

enum class trigger_event { insert, update, delete_ };

namespace detail {
// 表 sqlite_master 对应的结构体，用于查询数据库对象是否存在
struct sqlite_master_entry {
  std::string type;
  std::string name;
  std::string tbl_name;
  std::int32_t rootpage;
  std::string sql;
};
}  // namespace detail

struct sqlite_connection_t {
  sqlite3* db_{nullptr};
  sqlite_connection_t(sqlite3* in_db) : db_(in_db) {}
  ~sqlite_connection_t() { sqlite3_close(db_); }

  operator sqlite3*() const { return db_; }
};
using sqlite_connection_ptr = std::shared_ptr<sqlite_connection_t>;

struct sqlite_stmt {
  sqlite3_stmt* stmt_{nullptr};
  std::int32_t bind_index_{0};  // sqlite bind index starts from 1, but we use 0-based index internally
 public:
  sqlite_stmt() = default;
  explicit sqlite_stmt(const sqlite_connection_ptr& db, const std::string& sql);
  explicit sqlite_stmt(const session& s, const std::string& sql);
  ~sqlite_stmt();

  // dis copy
  sqlite_stmt(const sqlite_stmt&)            = delete;
  sqlite_stmt& operator=(const sqlite_stmt&) = delete;
  // dis move
  sqlite_stmt(sqlite_stmt&&)                 = delete;
  sqlite_stmt& operator=(sqlite_stmt&&)      = delete;

  void prepare(const sqlite_connection_ptr& db, const std::string& sql);
  void prepare(const session& s, const std::string& sql);
  // 重置绑定参数索引，以便重用 sqlite_stmt 对象执行多次 SQL 语句
  void reset_bind();
  std::int32_t get_bind_index();
  std::int64_t get_column_count() const;
  bool column_is_null(int columnIndex) const;
  std::int64_t get_last_insert_rowid() const;

  void step();
  std::int32_t step_not_throw();

  template <typename T>
  T get_column_value(int columnIndex) const;
  template <typename T>
  void bind(const T& in_value);
};

class storage : public boost::noncopyable {
  struct pragma_t {
    void synchronous(std::int32_t in_sync);
    void journal_mode(journal_mode_t in_mode);
    void recursive_triggers(bool in_recursive);
    void foreign_keys(bool in_foreign_keys);
    void locking_mode(bool in_exclusive);
    std::int32_t user_version();
    void user_version(std::int32_t version);

    void run(std::string_view in_pragma_sql, bool in_value);
    void run(std::string_view in_pragma_sql, std::string_view in_value);
    void run(std::string_view in_pragma_sql, std::int32_t in_value);

   private:
    storage& s_;
    explicit pragma_t(storage& s) : s_(s) {};
    friend struct storage;
  };
  friend struct table_info;
  friend struct sqlite_stmt;
  friend struct select_t;
  friend struct insert_t;
  friend struct update_t;
  friend struct session;

  struct backup_t {
   private:
    sqlite3_backup* backup_{nullptr};
    sqlite_connection_ptr dest_db_{nullptr};
    sqlite_connection_ptr src_db_{};

   public:
    explicit backup_t(sqlite_connection_ptr dest_db, sqlite_connection_ptr src_db);
    std::int32_t step(int pages = -1);
    ~backup_t();

    // dis copy
    backup_t(const backup_t&)            = delete;
    backup_t& operator=(const backup_t&) = delete;

    backup_t(backup_t&&)                 = default;
    backup_t& operator=(backup_t&&)      = default;
  };

  std::vector<std::shared_ptr<table_info_base>> tables_;
  std::map<std::type_index, std::size_t> type_to_table_index_;
  std::vector<std::shared_ptr<create_trigger_t>> triggers_;

  struct timed_connection {
    sqlite_connection_ptr conn_;
    std::chrono::steady_clock::time_point idle_since_;
  };

  std::atomic_bool finalized_{false};
  FSys::path db_path_;

  pragma_t pragma_{*this};
  boost::lockfree::stack<timed_connection, boost::lockfree::capacity<512>> connection_queue_{};
  std::atomic_char16_t thread_db_count_{0};
  bool is_opened_{false};

 protected:
  sqlite3* only_open_db();

  sqlite_connection_ptr get_thread_db();
  void add_thread_db(const sqlite_connection_ptr& in_ptr);

  virtual void open_(FSys::path in_path, std::int32_t in_flags);
  // 注册自定义扩展
  virtual void register_custom_extension(sqlite3* in_sqlite);

 public:
  storage() = default;
  ~storage();

  virtual void open(FSys::path in_path, std::int32_t in_flags) final;
  virtual void open() final;
  virtual void open(const FSys::path& in_path) final;

  void sync_schema();
  pragma_t& pragma();

  static fts5_api* get_fts5_api(sqlite3* in_sqlite);

  template <typename T>
  table_info& reg_table(std::string&& in_name);
  template <typename T>
  table_fts_info& reg_virtual_table(std::string&& in_name);
  create_trigger_t create_trigger(std::string in_name);

  backup_t backup(const FSys::path& dest_path);
  void backup_to(const FSys::path& dest_path) {
    auto backup = this->backup(dest_path);
    backup.step(-1);
  }
  session create_session();

  template <typename T>
  bool has_reg_table();

  template <typename T>
  std::string get_column_name(auto T::* in_ptr, const to_sql_ctx& ctx) const;
  std::string get_column_name(const table_columns_t& in_column, const to_sql_ctx& ctx) const;

  template <typename T>
  const std::vector<column_info>& get_table_columns() const;
  template <typename T>
  std::string get_table_name() const;
  std::string get_table_name(std::type_index in_type_index) const;
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
