//
// Created by TD on 2022/6/2.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>
#include <utility>
namespace doodle::database_n {
class sqlite_client {
 public:
  void open_sqlite(const FSys::path& in_path, bool only_ctx = false);
  void update_entt();
  void create_sqlite();
};

class file_translator;
using file_translator_ptr = std::shared_ptr<file_translator>;
class file_translator : public std::enable_shared_from_this<file_translator> {
 private:
  bool open_init(const FSys::path& in_path);
  bool open_next();
  bool open_end();

 protected:
  virtual bool open_init_impl(const FSys::path& in_path) = 0;
  virtual bool open_next_impl()                          = 0;
  virtual bool open_end_impl();

  //  virtual bool save_impl(const FSys::path& in_path) = 0;

  // public:
  class async_open_impl {
   public:
    boost::asio::high_resolution_timer& timer_attr;
    file_translator_ptr file_translator_attr;
    explicit async_open_impl(
        boost::asio::high_resolution_timer& in_timer,
        file_translator_ptr in_file_translator)
        : timer_attr(in_timer), file_translator_attr(std::move(in_file_translator)) {}

    template <typename Self>
    void operator()(Self& self,
                    boost::system::error_code error = {}) {
    }
  };

 public:
  virtual ~file_translator() = default;
  /**
   * @brief 使用路径打开项目文件
   * @param in_path 传入的项目文件路径
   */

  template <typename CompletionToken>
  auto async_open(const FSys::path& in_path, CompletionToken&& token)
      ->
      typename boost::asio::async_result<
          typename std::decay_t<CompletionToken>,
          void(bsys::error_code)>::return_type {
    boost::asio::high_resolution_timer l_time{g_io_context()};

    return boost::asio::async_compose<CompletionToken,
                                      void(bsys::error_code)>(
        async_open_impl{l_time, this->shared_from_this()}, token, l_time);
  };
};

class sqlite_file : public file_translator {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 protected:
  virtual bool open_impl(const FSys::path& in_path) override;
  //  virtual bool save_impl(const FSys::path& in_path) override;

 public:
  sqlite_file();
  explicit sqlite_file(registry_ptr in_registry);
  virtual ~sqlite_file();

  sqlite_file(const sqlite_file& in) noexcept            = delete;
  sqlite_file& operator=(const sqlite_file& in) noexcept = delete;

  sqlite_file(sqlite_file&& in) noexcept;
  sqlite_file& operator=(sqlite_file&& in) noexcept;
};

}  // namespace doodle::database_n
