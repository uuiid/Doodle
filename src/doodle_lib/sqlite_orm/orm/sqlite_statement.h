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
  std::int32_t bind(sqlite3_stmt* stmt, int index, const T& value) const {
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
  T extract(sqlite3_stmt* stmt, int columnIndex) const {
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
  const column_type operator()() const {
    static column_type type = []() {
      if constexpr (std::is_integral_v<T>) {
        return column_type::integer;
      } else if constexpr (std::is_floating_point_v<T>) {
        return column_type::real;
      }
      return column_type::null;
    }();
    return type;
  }
};

// 字符串特化
template <>
struct sqlite_statement_binder<std::string> {
  std::int32_t bind(sqlite3_stmt* stmt, int index, const std::string& value) const {
    return sqlite3_bind_text(stmt, index, value.c_str(), value.size(), SQLITE_TRANSIENT);
  }
};

template <>
struct sqlite_statement_extractor<std::string> {
  std::string extract(sqlite3_stmt* stmt, int columnIndex) const {
    const unsigned char* text = sqlite3_column_text(stmt, columnIndex);
    return text ? reinterpret_cast<const char*>(text) : "";
  }
};

template <>
struct sqlite_statement_printer<std::string> {
  const column_type operator()() const {
    static column_type type = column_type::text;
    return type;
  }
};
// char* 特化
template <>
struct sqlite_statement_binder<const char*> {
  std::int32_t bind(sqlite3_stmt* stmt, int index, const char* value) const {
    return sqlite3_bind_text(stmt, index, value, -1, SQLITE_TRANSIENT);
  }
};

// uuid特化
template <>
struct sqlite_statement_binder<uuid> {
  std::int32_t bind(sqlite3_stmt* stmt, int index, const uuid& value) const {
    return sqlite3_bind_blob(stmt, index, value.data(), value.size(), SQLITE_TRANSIENT);
  }
};
template <>
struct sqlite_statement_extractor<uuid> {
  uuid extract(sqlite3_stmt* stmt, int columnIndex) const {
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
  const column_type operator()() const {
    static column_type type = column_type::blob;
    return type;
  }
};

// 时间点特化
template <typename Clock, typename Duration>
struct sqlite_statement_binder<std::chrono::time_point<Clock, Duration>>
    : sqlite_statement_binder<typename Duration::rep> {
  int bind(sqlite3_stmt* stmt, int index, const std::chrono::time_point<Clock, Duration>& value) const {
    return sqlite_statement_binder<typename Duration::rep>::bind(stmt, index, value.time_since_epoch().count());
  }
};
template <typename Clock, typename Duration>
struct sqlite_statement_extractor<std::chrono::time_point<Clock, Duration>>
    : sqlite_statement_extractor<typename Duration::rep> {
  std::chrono::time_point<Clock, Duration> extract(sqlite3_stmt* stmt, int columnIndex) const {
    const auto l_value = sqlite_statement_extractor<typename Duration::rep>::extract(stmt, columnIndex);
    return std::chrono::time_point<Clock, Duration>{Duration{l_value}};
  }
};
template <typename Clock, typename Duration>
struct sqlite_statement_printer<std::chrono::time_point<Clock, Duration>> {
  const column_type operator()() const {
    if constexpr (std::is_floating_point_v<typename Duration::rep>) {
      return column_type::real;
    } else if constexpr (std::is_integral_v<typename Duration::rep>) {
      return column_type::integer;
    }
  }
};

// nlohmann::json特化
template <>
struct sqlite_statement_binder<nlohmann::json> : sqlite_statement_binder<std::string> {
  int bind(sqlite3_stmt* stmt, int index, const nlohmann::json& value) const {
    auto l_json_str = value.dump();
    return sqlite_statement_binder<std::string>::bind(stmt, index, l_json_str);
  }
};
template <>
struct sqlite_statement_extractor<nlohmann::json> : sqlite_statement_extractor<std::string> {
  nlohmann::json extract(sqlite3_stmt* stmt, int columnIndex) const {
    const auto l_str = sqlite_statement_extractor<std::string>::extract(stmt, columnIndex);
    return l_str.empty() ? nlohmann::json{} : nlohmann::json::parse(l_str);
  }
};
template <>
struct sqlite_statement_printer<nlohmann::json> : sqlite_statement_printer<std::string> {};

// FSys::path特化
template <>
struct sqlite_statement_binder<FSys::path> : sqlite_statement_binder<std::string> {
  int bind(sqlite3_stmt* stmt, int index, const FSys::path& value) const {
    auto l_path_str = value.generic_string();
    return sqlite_statement_binder<std::string>::bind(stmt, index, l_path_str);
  }
};
template <>
struct sqlite_statement_extractor<FSys::path> : sqlite_statement_extractor<std::string> {
  FSys::path extract(sqlite3_stmt* stmt, int columnIndex) const {
    const auto l_str = sqlite_statement_extractor<std::string>::extract(stmt, columnIndex);
    return FSys::path{l_str};
  }
};
template <>
struct sqlite_statement_printer<FSys::path> : sqlite_statement_printer<std::string> {};


// std::chrono::zoned_time
template <typename Duration>
struct sqlite_statement_binder<std::chrono::zoned_time<Duration>> : sqlite_statement_binder<std::string> {
  int bind(sqlite3_stmt* stmt, int index, const std::chrono::zoned_time<Duration>& value) const {
    try {
      return sqlite_statement_binder<std::string>::bind(stmt, index, fmt::to_string(value.get_sys_time()));
    } catch (fmt::format_error& err) {
      SPDLOG_ERROR(err.what());
      auto l_new = std::chrono::system_clock::now();
      return sqlite_statement_binder<std::string>::bind(stmt, index, fmt::to_string(l_new));
    }
  }
};
template <typename Duration>
struct sqlite_statement_extractor<std::chrono::zoned_time<Duration>> : sqlite_statement_extractor<std::string> {
  std::chrono::zoned_time<Duration> extract(sqlite3_stmt* stmt, int columnIndex) const {
    const auto l_str = sqlite_statement_extractor<std::string>::extract(stmt, columnIndex);
    std::istringstream l_istr{l_str};
    std::chrono::time_point<std::chrono::system_clock, Duration> l_value =
        std::chrono::time_point_cast<Duration>(std::chrono::system_clock::now());
    if (l_istr >> parse("%F %T", l_value))
      ;
    else if (l_istr.clear(), l_istr.str(l_str), l_istr >> parse("%F", l_value))
      ;
    else
      throw std::runtime_error(fmt::format("Failed to parse zoned_time from string: {}", l_str));
    return std::chrono::zoned_time{std::chrono::current_zone(), l_value};
  }
};
template <typename Duration>
struct sqlite_statement_printer<std::chrono::zoned_time<Duration>> : sqlite_statement_printer<std::string> {};
}  // namespace doodle::orm