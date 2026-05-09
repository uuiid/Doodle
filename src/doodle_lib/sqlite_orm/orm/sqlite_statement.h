#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <sqlite3.h>

namespace doodle::orm {
// 这里定义了 sqlite_statement_binder, sqlite_statement_extractor, sqlite_statement_printer
// 的特化版本，用于绑定、提取和打印不同类型的数据

// 数字特化
template <typename T>
  requires std::is_arithmetic_v<T>
struct sqlite_statement_binder<T> {
  std::int32_t bind(sqlite3_stmt* stmt, int index, const T& value) {
    if constexpr (std::is_integral_v<T>) {
      return sqlite3_bind_int64(stmt, index, static_cast<std::int64_t>(value));
    } else if constexpr (std::is_floating_point_v<T>) {
      return sqlite3_bind_double(stmt, index, static_cast<std::double_t>(value));
    }
  }
};

template <typename T>
  requires std::is_arithmetic_v<T>
struct sqlite_statement_extractor<T> {
  T extract(sqlite3_stmt* stmt, int columnIndex) {
    if constexpr (std::is_integral_v<T>) {
      return static_cast<T>(sqlite3_column_int64(stmt, columnIndex));
    } else if constexpr (std::is_floating_point_v<T>) {
      return static_cast<T>(sqlite3_column_double(stmt, columnIndex));
    }
  }
};

template <typename T>
  requires std::is_arithmetic_v<T>
struct sqlite_statement_printer<T> {
  const std::string& operator()() const {
    static std::string name = []() {
      if constexpr (std::is_integral_v<T>) {
        return "INTEGER";
      } else if constexpr (std::is_floating_point_v<T>) {
        return "REAL";
      }
      return "UNKNOWN";
    }();
    return name;
  }
};

// 字符串特化
template <>
struct sqlite_statement_binder<std::string> {
  std::int32_t bind(sqlite3_stmt* stmt, int index, const std::string& value) {
    return sqlite3_bind_text(stmt, index, value.c_str(), value.size(), SQLITE_TRANSIENT);
  }
};

template <>
struct sqlite_statement_extractor<std::string> {
  std::string extract(sqlite3_stmt* stmt, int columnIndex) {
    const unsigned char* text = sqlite3_column_text(stmt, columnIndex);
    return text ? reinterpret_cast<const char*>(text) : "";
  }
};

template <>
struct sqlite_statement_printer<std::string> {
  const std::string& operator()() const {
    static std::string name = "TEXT";
    return name;
  }
};
// uuid特化
template <>
struct sqlite_statement_binder<uuid> {
  std::int32_t bind(sqlite3_stmt* stmt, int index, const uuid& value) {
    return sqlite3_bind_blob(stmt, index, value.data(), value.size(), SQLITE_TRANSIENT);
  }
};
template <>
struct sqlite_statement_extractor<uuid> {
  uuid extract(sqlite3_stmt* stmt, int columnIndex) {
    const void* blob = sqlite3_column_blob(stmt, columnIndex);
    int size         = sqlite3_column_bytes(stmt, columnIndex);
    if (blob && size == sizeof(uuid)) {
      uuid value;
      std::memcpy(&value, blob, sizeof(uuid));
      return value;
    }
    return uuid{};
  }
};
template <>
struct sqlite_statement_printer<uuid> {
  const std::string& operator()() const {
    static std::string name = "BLOB";
    return name;
  }
};
}  // namespace doodle::orm