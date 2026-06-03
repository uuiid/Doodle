#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <boost/unordered/concurrent_flat_map.hpp>

#include <atomic>
#include <fmt/format.h>
#include <functional>
#include <map>
#include <memory>
#include <sqlite3.h>
#include <string>
#include <string_view>
#include <thread>
#include <typeindex>
#include <vector>

namespace boost {
// 为 std::thread::id 提供哈希函数
template <>
struct hash<std::thread::id> {
  std::size_t operator()(const std::thread::id& id) const noexcept { return std::hash<std::thread::id>{}(id); }
};
}  // namespace boost

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
  std::vector<std::string> get_foreign_key_create_sql(storage& s, const to_sql_ctx& ctx) const;
  virtual std::string to_sql(storage& s, const to_sql_ctx& ctx) const = 0;

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

  std::string to_sql(storage& s, const to_sql_ctx& ctx) const override;
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

  std::string to_sql(storage& s, const to_sql_ctx& ctx) const override;
};

enum class trigger_timing { before, after, instead_of };

enum class trigger_event { insert, update, delete_ };

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
  bool column_is_null(int columnIndex) const;

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

  struct backup_t {
   private:
    sqlite3_backup* backup_{nullptr};
    sqlite3* dest_db_{nullptr};
    sqlite3* src_db_{nullptr};

   public:
    explicit backup_t(sqlite3* dest_db, sqlite3* src_db);
    std::int32_t step(int pages = -1);
    ~backup_t();
  };

  std::vector<std::shared_ptr<table_info_base>> tables_;
  std::map<std::type_index, std::size_t> type_to_table_index_;
  std::vector<std::shared_ptr<create_trigger_t>> triggers_;

  std::atomic_bool finalized_{false};
  FSys::path db_path_;

  sqlite3* db_{nullptr};
  pragma_t pragma_{*this};
  // 每个线程对应一个 sqlite3 连接，以避免多线程访问同一个连接导致的竞争问题
  boost::unordered::concurrent_flat_map<std::thread::id, sqlite3*> thread_db_map_;

 protected:
  sqlite3* only_open_db();

  sqlite3* get_thread_db();
  virtual void open_(FSys::path in_path, std::int32_t in_flags);
  // 注册自定义扩展
  virtual void register_custom_extension(sqlite3* in_sqlite);

 public:
  storage() = default;
  ~storage();

  void open(FSys::path in_path, std::int32_t in_flags);
  void open();
  void open(const FSys::path& in_path);

  void sync_schema();
  pragma_t& pragma();

  fts5_api* get_fts5_api(sqlite3* in_sqlite) const;

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
  std::int64_t get_last_insert_rowid() const;

  // 删除表
  void drop_table(const std::string& table_name);
  // 删除索引
  void drop_index(const std::string& index_name);
  // 删除触发器
  void drop_trigger(const std::string& trigger_name);
  // 删除view
  void drop_view(const std::string& view_name);
  // 检查表是否存在
  bool table_exists(const std::string& table_name);
  // 检查索引是否存在
  bool index_exists(const std::string& index_name);
  // 检查触发器是否存在
  bool trigger_exists(const std::string& trigger_name);
  // vacuum数据库
  void vacuum();
  // 运行任意SQL
  void exec(std::string_view sql);
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
