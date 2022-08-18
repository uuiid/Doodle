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
  bsys::error_code open(const FSys::path& in_path);

  bsys::error_code open_end();

 protected:
  /**
   * @brief  初始化文件加载
   * @param in_path 传入的保存路径
   * @return 错误代码(异步)
   */
  virtual bsys::error_code open_impl(const FSys::path& in_path) = 0;

 private:
  bsys::error_code save(const FSys::path& in_path);
  bsys::error_code save_end();

 protected:
  virtual bsys::error_code save_impl(const FSys::path& in_path) = 0;

  //  virtual bool save_impl(const FSys::path& in_path) = 0;

  // public:
  class async_open_impl {
   public:
    enum class state : std::uint8_t {

      init,
      next,
      end
    };
    boost::asio::high_resolution_timer& timer_attr;
    file_translator_ptr file_translator_attr;
    state state_attr;
    FSys::path file_path;

    explicit async_open_impl(
        FSys::path in_file_path,
        boost::asio::high_resolution_timer& in_timer,
        file_translator_ptr in_file_translator)
        : timer_attr(in_timer),
          file_translator_attr(std::move(in_file_translator)),
          state_attr(state::init),
          file_path(std::move(in_file_path)) {}

    template <typename Self>
    void operator()(Self& self,
                    boost::system::error_code error = {}) {
      switch (self.state_attr) {
        case state::init: {
          error = self->file_translator_attr->open_init(file_path);
          if (error) self.complete(error);
          self->state_attr = state::init;
          break;
        }

        case state::next: {
          bsys::result<bool> l_r = self->file_translator_attr->open_next();
          if (l_r.has_error()) self.complete(error);
          if (l_r.has_value() && l_r.value())
            self->state_attr = state::next;
          else
            self->state_attr = state::end;
          break;
        }
        case state::end: {
          error = self->file_translator_attr->open_end();

          self.complete(error);
          break;
        }
      }
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
        async_open_impl{in_path, l_time, this->shared_from_this()}, token, l_time);
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
