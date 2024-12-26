//
// Created by TD on 24-9-25.
//

#pragma once

#define DOODLE_SQLITE_ENUM_TYPE_(enum_type)                                                                          \
  template <>                                                                                                        \
  struct type_printer<enum_type> : public text_printer {};                                                           \
                                                                                                                     \
  template <>                                                                                                        \
  struct statement_binder<enum_type> {                                                                               \
    int bind(sqlite3_stmt* stmt, int index, const enum_type& value) {                                                \
      return statement_binder<std::string_view>().bind(stmt, index, magic_enum::enum_name(value));                   \
    }                                                                                                                \
  };                                                                                                                 \
                                                                                                                     \
  template <>                                                                                                        \
  struct field_printer<enum_type> {                                                                                  \
    std::string operator()(const enum_type& t) const { return std::string{magic_enum::enum_name(t)}; }               \
  };                                                                                                                 \
                                                                                                                     \
  template <>                                                                                                        \
  struct row_extractor<enum_type> {                                                                                  \
    enum_type extract(const char* columnText) const { return magic_enum::enum_cast<enum_type>(columnText).value(); } \
    enum_type extract(sqlite3_stmt* stmt, int columnIndex) const {                                                   \
      const auto str = sqlite3_column_text(stmt, columnIndex);                                                       \
      return this->extract(reinterpret_cast<const char*>(str));                                                      \
    }                                                                                                                \
  };

template <typename T>
std::string enum_array_to_string(const T& t) {
  std::string l_ret{'{'};
  for (const auto& v : t) {
    l_ret += fmt::format(", {}", magic_enum::enum_name(v));
  }
  l_ret += '}';
  return l_ret;
}

template <typename T>
std::vector<T> string_to_enum_array(const std::string& t) {
  std::vector<T> l_ret;
  if (t.empty()) return l_ret;
  auto l_begin = ++t.begin();
  while (l_begin != t.end()) {
    auto l_end = std::find(l_begin, t.end(), ',');
    l_ret.emplace_back(magic_enum::enum_cast<T>(std::string_view{l_begin, l_end}).value());
    l_begin = l_end + 1;
  }
  return l_ret;
}

#define DOODLE_SQLITE_ENUM_ARRAY_TYPE_(enum_type)                                                                  \
  template <>                                                                                                      \
  struct type_printer<std::vector<enum_type>> : public text_printer {};                                            \
                                                                                                                   \
  template <>                                                                                                      \
  struct statement_binder<std::vector<enum_type>> {                                                                \
    int bind(sqlite3_stmt* stmt, int index, const std::vector<enum_type>& value) {                                 \
      return statement_binder<std::string_view>().bind(stmt, index, enum_array_to_string(value));                  \
    }                                                                                                              \
  };                                                                                                               \
                                                                                                                   \
  template <>                                                                                                      \
  struct field_printer<std::vector<enum_type>> {                                                                   \
    std::string operator()(const std::vector<enum_type>& t) const { return std::string{enum_array_to_string(t)}; } \
  };                                                                                                               \
                                                                                                                   \
  template <>                                                                                                      \
  struct row_extractor<std::vector<enum_type>> {                                                                   \
    std::vector<enum_type> extract(const char* columnText) const {                                                 \
      return string_to_enum_array<enum_type>(columnText);                                                          \
    }                                                                                                              \
    std::vector<enum_type> extract(sqlite3_stmt* stmt, int columnIndex) const {                                    \
      const auto str = sqlite3_column_text(stmt, columnIndex);                                                     \
      return this->extract(reinterpret_cast<const char*>(str));                                                    \
    }                                                                                                              \
  };

#define DOODLE_GET_BY_PARENT_ID_SQL(class_name)                                                \
  template <>                                                                                  \
  std::vector<class_name> sqlite_database::get_by_parent_id<class_name>(const uuid& in_uuid) { \
    return impl_->get_by_parent_id<class_name>(in_uuid);                                       \
  }

#define DOODLE_UUID_TO_ID(class_name)                                         \
  template <>                                                                 \
  std::int64_t sqlite_database::uuid_to_id<class_name>(const uuid& in_uuid) { \
    return impl_->uuid_to_id<class_name>(in_uuid);                            \
  }

#define DOODLE_GET_BY_UUID_SQL(class_name)                                                \
  template <>                                                                             \
  std::vector<class_name> sqlite_database::get_by_uuid<class_name>(const uuid& in_uuid) { \
    return impl_->get_by_uuid<class_name>(in_uuid);                                       \
  }

#define DOODLE_GET_ALL_SQL(class_name)                 \
  template <>                                          \
  std::vector<class_name> sqlite_database::get_all() { \
    return impl_->get_all<class_name>();               \
  }

#define DOODLE_INSTALL_SQL(class_name)                                                                \
  template <>                                                                                         \
  boost::asio::awaitable<void> sqlite_database::install(const std::shared_ptr<class_name>& in_data) { \
    return impl_->install<class_name>(in_data);                                                       \
  }

#define DOODLE_INSTALL_RANGE(class_name)                       \
  template <>                                                  \
  boost::asio::awaitable<void> sqlite_database::install_range( \
      const std::shared_ptr<std::vector<class_name>>& in_data  \
  ) {                                                          \
    return impl_->install_range<class_name>(in_data);          \
  }

#define DOODLE_REMOVE_RANGE(class_name)                             \
  template <>                                                       \
  boost::asio::awaitable<void> sqlite_database::remove<class_name>( \
      const std::shared_ptr<std::vector<std::int64_t>>& in_data     \
  ) {                                                               \
    return impl_->remove<class_name>(in_data);                      \
  }

#define DOODLE_REMOVE_BY_UUID(class_name)                                                                  \
  template <>                                                                                              \
  boost::asio::awaitable<void> sqlite_database::remove<class_name>(const std::shared_ptr<uuid>& in_data) { \
    return impl_->remove<class_name>(in_data);                                                             \
  }