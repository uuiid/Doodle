#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio/prepend.hpp>

#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <string_view>
#include <utility>
namespace doodle {

namespace details {
/**
 * @brief 这个是sql连接类, 负责配置和生成sql链接
 *
 */
class DOODLE_CORE_API database_info {
 public:
  static constexpr std::string_view memory_data{":memory:"};
  FSys::path path_;
  database_info() : database_info(memory_data){};
  explicit database_info(FSys::path in_path) : path_(std::move(in_path)){};

  [[nodiscard]] sql_connection_ptr get_connection() const;
  [[nodiscard]] sql_connection_ptr get_connection_const() const;
};

class DOODLE_CORE_API database_pool_info {
  void create_pool(const std::string& in_path);
  std::shared_ptr<sqlpp::sqlite3::connection_pool> pool_;
  FSys::path path_;

 public:
  static constexpr std::string_view memory_data{":memory:"};

  database_pool_info() : database_pool_info(memory_data){};
  explicit database_pool_info(FSys::path in_path) : path_(std::move(in_path)) { create_pool(path_.generic_string()); };

  // path
  inline void set_path(const FSys::path& in_path) {
    path_ = in_path;
    create_pool(in_path.generic_string());
  }
  [[nodiscard]] inline const FSys::path& get_path() const { return path_; }

  [[nodiscard]] sql_connection_ptr get_connection() const;

  // async_install_db
  template <typename T, typename CompletionHandler>
  [[nodiscard]] auto async_install_db(T in_args, CompletionHandler&& in_handler) {
    boost::asio::post(g_io_context(), [this, in_args, in_handler = std::forward<CompletionHandler>(in_handler)]() {
      auto l_conn = get_connection();
      boost::system::error_code l_ec{};
      try {
        in_args->install_db(l_conn);
      } catch (const sqlpp::exception& e) {
        l_ec = error_enum::sqlite3_save_error;
      }
      boost::asio::post(boost::asio::prepend(std::move(in_handler), l_ec, in_args));
    });
  }
  // async_update_db
  template <typename T, typename CompletionHandler>
  [[nodiscard]] auto async_update_db(T in_args, CompletionHandler&& in_handler) {
    boost::asio::post(g_io_context(), [this, in_args, in_handler = std::forward<CompletionHandler>(in_handler)]() {
      auto l_conn = get_connection();
      boost::system::error_code l_ec{};
      try {
        in_args->update_db(l_conn);
      } catch (const sqlpp::exception& e) {
        l_ec = error_enum::sqlite3_save_error;
      }
      boost::asio::post(boost::asio::prepend(std::move(in_handler), l_ec, in_args));
    });
  }
  // async_select_db
  template <typename T, typename CompletionHandler>
  [[nodiscard]] auto async_select_db(T in_args, CompletionHandler&& in_handler) {
    boost::asio::post(g_io_context(), [this, in_args, in_handler = std::forward<CompletionHandler>(in_handler)]() {
      auto l_conn = get_connection();
      boost::system::error_code l_ec{};
      try {
        in_args->select_db(l_conn);
      } catch (const sqlpp::exception& e) {
        l_ec = error_enum::sqlite3_save_error;
      }
      boost::asio::post(boost::asio::prepend(std::move(in_handler), l_ec, in_args));
    });
  }
  // async_delete_db
  template <typename T, typename CompletionHandler>
  [[nodiscard]] auto async_delete_db(T in_args, CompletionHandler&& in_handler) {
    boost::asio::post(g_io_context(), [this, in_args, in_handler = std::forward<CompletionHandler>(in_handler)]() {
      auto l_conn = get_connection();
      boost::system::error_code l_ec{};
      try {
        in_args->delete_db(l_conn);
      } catch (const sqlpp::exception& e) {
        l_ec = error_enum::sqlite3_save_error;
      }
      boost::asio::post(boost::asio::prepend(std::move(in_handler), l_ec, in_args));
    });
  }
  // async_select_all
  template <typename T, typename CompletionHandler>
  [[nodiscard]] auto async_select_all(CompletionHandler&& in_handler) {
    boost::asio::post(g_io_context(), [this, in_handler = std::forward<CompletionHandler>(in_handler)]() {
      auto l_conn = get_connection();
      boost::system::error_code l_ec{};
      std::vector<T> l_args{};
      try {
        l_args = T::select_all(l_conn);
      } catch (const sqlpp::exception& e) {
        l_ec = error_enum::sqlite3_save_error;
      }
      boost::asio::post(boost::asio::prepend(std::move(in_handler), l_ec, l_args));
    });
  }
};

}  // namespace details

}  // namespace doodle
