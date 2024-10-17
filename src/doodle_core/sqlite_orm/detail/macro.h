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

#define DOODLE_GET_BY_KITSU_UUID_SQL(class_name)                                                                  \
  template <>                                                                                                     \
  std::vector<class_name> sqlite_database::get_by_kitsu_uuid<class_name>(const uuid& in_uuid) {                   \
    using namespace sqlite_orm;                                                                                   \
    auto l_storage = get_cast_storage(storage_any_);                                                              \
    return l_storage->get_all<class_name>(sqlite_orm::where(sqlite_orm::c(&class_name::kitsu_uuid_) == in_uuid)); \
  }

#define DOODLE_GET_BY_PARENT_ID_SQL(class_name)                                                                    \
  template <>                                                                                                      \
  std::vector<class_name> sqlite_database::get_by_parent_id<class_name>(const uuid& in_uuid) {                     \
    using namespace sqlite_orm;                                                                                    \
    auto l_storage = get_cast_storage(storage_any_);                                                               \
    return l_storage->get_all<class_name>(sqlite_orm::where(sqlite_orm::c(&class_name::uuid_parent_) == in_uuid)); \
  }

#define DOODLE_UUID_TO_ID(class_name)                                                                                    \
  template <>                                                                                                            \
  std::int64_t sqlite_database::uuid_to_id<class_name>(const uuid& in_uuid) {                                            \
    using namespace sqlite_orm;                                                                                          \
    auto l_storage = get_cast_storage(storage_any_);                                                                     \
    auto l_v       = l_storage->get_all<class_name>(sqlite_orm::where(sqlite_orm::c(&class_name::uuid_id_) == in_uuid)); \
    return l_v.empty() ? 0 : l_v[0].id_;                                                                                 \
  }

#define DOODLE_GET_BY_UUID_SQL(class_name)                                                                     \
  template <>                                                                                                  \
  std::vector<class_name> sqlite_database::get_by_uuid<class_name>(const uuid& in_uuid) {                      \
    using namespace sqlite_orm;                                                                                \
    auto l_storage = get_cast_storage(storage_any_);                                                           \
    return l_storage->get_all<class_name>(sqlite_orm::where(sqlite_orm::c(&class_name::uuid_id_) == in_uuid)); \
  }

#define DOODLE_GET_ALL_SQL(class_name)                 \
  template <>                                          \
  std::vector<class_name> sqlite_database::get_all() { \
    auto l_storage = get_cast_storage(storage_any_);   \
    return l_storage->get_all<class_name>();           \
  }

#define DOODLE_INSTALL_SQL(class_type)                                                                                 \
  template <>                                                                                                          \
  boost::asio::awaitable<tl::expected<void, std::string>> sqlite_database::install(std::shared_ptr<class_type> in_data \
  ) {                                                                                                                  \
    DOODLE_TO_SQLITE_THREAD();                                                                                         \
    tl::expected<void, std::string> l_ret{};                                                                           \
    try {                                                                                                              \
      auto l_storage = get_cast_storage(storage_any_);                                                                 \
      auto l_g       = l_storage->transaction_guard();                                                                 \
      if (in_data->id_ == 0)                                                                                           \
        in_data->id_ = l_storage->insert<class_type>(*in_data);                                                        \
      else {                                                                                                           \
        l_storage->replace<class_type>(*in_data);                                                                      \
      }                                                                                                                \
      l_g.commit();                                                                                                    \
    } catch (...) {                                                                                                    \
      l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());                                  \
    }                                                                                                                  \
    DOODLE_TO_SELF();                                                                                                  \
    co_return l_ret;                                                                                                   \
  }

#define DOODLE_INSTALL_RANGE(class_name)                                                                               \
  template <>                                                                                                          \
  boost::asio::awaitable<tl::expected<void, std::string>> sqlite_database::install_range(                              \
      std::shared_ptr<std::vector<class_name>> in_data                                                                 \
  ) {                                                                                                                  \
    if (!std::is_sorted(in_data->begin(), in_data->end(), [](const auto& in_r, const auto& in_l) {                     \
          return in_r.id_ < in_l.id_;                                                                                  \
        }))                                                                                                            \
      std::sort(in_data->begin(), in_data->end(), [](const auto& in_r, const auto& in_l) {                             \
        return in_r.id_ < in_l.id_;                                                                                    \
      });                                                                                                              \
    std::size_t l_split =                                                                                              \
        std::distance(in_data->begin(), std::ranges::find_if(*in_data, [](const auto& in_) { return in_.id_ != 0; })); \
                                                                                                                       \
    DOODLE_TO_SQLITE_THREAD();                                                                                         \
    tl::expected<void, std::string> l_ret{};                                                                           \
    try {                                                                                                              \
      auto l_storage = get_cast_storage(storage_any_);                                                                 \
      auto l_g       = l_storage->transaction_guard();                                                                 \
      for (std::size_t i = 0; i < l_split;) {                                                                          \
        auto l_end = std::min(i + g_step_size, l_split);                                                               \
        l_storage->insert_range<class_name>(in_data->begin() + i, in_data->begin() + l_end);                           \
        i = l_end;                                                                                                     \
      }                                                                                                                \
                                                                                                                       \
      for (std::size_t i = l_split; i < in_data->size();) {                                                            \
        auto l_end = std::min(i + g_step_size, in_data->size());                                                       \
        l_storage->replace_range<class_name>(in_data->begin() + i, in_data->begin() + l_end);                          \
        i = l_end;                                                                                                     \
      }                                                                                                                \
      l_g.commit();                                                                                                    \
                                                                                                                       \
      for (std::size_t i = 0; i < l_split; ++i) {                                                                      \
        using namespace sqlite_orm;                                                                                    \
        auto l_v = l_storage->select(                                                                                  \
            &class_name::id_, sqlite_orm::where(c(&class_name::uuid_id_) == (*in_data)[i].uuid_id_)                    \
        );                                                                                                             \
        if (!l_v.empty()) (*in_data)[i].id_ = l_v.front();                                                             \
      }                                                                                                                \
                                                                                                                       \
    } catch (...) {                                                                                                    \
      l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());                                  \
    }                                                                                                                  \
    DOODLE_TO_SELF();                                                                                                  \
    co_return l_ret;                                                                                                   \
  }

#define DOODLE_REMOVE_RANGE(class_name)                                                                 \
  template <>                                                                                           \
  boost::asio::awaitable<tl::expected<void, std::string>> sqlite_database::remove<class_name>(          \
      std::shared_ptr<std::vector<std::int64_t>> in_data                                                \
  ) {                                                                                                   \
    DOODLE_TO_SQLITE_THREAD();                                                                          \
    tl::expected<void, std::string> l_ret{};                                                            \
                                                                                                        \
    try {                                                                                               \
      auto l_storage = get_cast_storage(storage_any_);                                                  \
      auto l_g       = l_storage->transaction_guard();                                                  \
      l_storage->remove_all<class_name>(sqlite_orm::where(sqlite_orm::in(&class_name::id_, *in_data))); \
      l_g.commit();                                                                                     \
    } catch (...) {                                                                                     \
      l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());                   \
    }                                                                                                   \
    DOODLE_TO_SELF();                                                                                   \
    co_return l_ret;                                                                                    \
  }

#define DOODLE_REMOVE_BY_UUID(class_name)                                                                              \
  template <>                                                                                                          \
  boost::asio::awaitable<tl::expected<void, std::string>> sqlite_database::remove<class_name>(                         \
      std::shared_ptr<uuid> in_data                                                                                    \
  ) {                                                                                                                  \
    DOODLE_TO_SQLITE_THREAD();                                                                                         \
    tl::expected<void, std::string> l_ret{};                                                                           \
                                                                                                                       \
    try {                                                                                                              \
      auto l_storage = get_cast_storage(storage_any_);                                                                 \
      auto l_g       = l_storage->transaction_guard();                                                                 \
      l_storage->remove_all<class_name>(sqlite_orm::where(sqlite_orm::c(&assets_helper::database_t::id_) = *in_data)); \
      l_g.commit();                                                                                                    \
    } catch (...) {                                                                                                    \
      l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());                                  \
    }                                                                                                                  \
    DOODLE_TO_SELF();                                                                                                  \
    co_return l_ret;                                                                                                   \
  }