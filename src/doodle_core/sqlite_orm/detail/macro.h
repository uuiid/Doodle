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

