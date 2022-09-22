//
// Created by TD on 2022/6/2.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_core/logger/logger.h>
#include <boost/asio.hpp>
#include <utility>
namespace doodle::database_n {

class file_translator;
using file_translator_ptr = std::shared_ptr<file_translator>;
class DOODLE_CORE_API file_translator : public std::enable_shared_from_this<file_translator> {
 private:
  bsys::error_code open_begin(const FSys::path& in_path);
  bsys::error_code open(const FSys::path& in_path);
  bsys::error_code open_end();

  bsys::error_code save_begin(const FSys::path& in_path);
  bsys::error_code save(const FSys::path& in_path);
  bsys::error_code save_end();

 protected:
  /**
   * @brief  文件加载(@b 非主线程) 可以阻塞,
   * @param in_path 传入的保存路径
   * @return 错误代码(异步)
   */
  virtual bsys::error_code open_impl(const FSys::path& in_path) = 0;
  /**
   * @brief 文件保存(@b 非主线程) 可以阻塞,
   * @param in_path 传入的需要保存的路径
   * @return 错误代码(异步)
   */
  virtual bsys::error_code save_impl(const FSys::path& in_path) = 0;

  //  virtual bool save_impl(const FSys::path& in_path) = 0;
  enum class state : std::uint8_t {
    init,
    end
  };

  template <typename CompletionToken>
  auto async_open_impl_fun(CompletionToken&& token, FSys::path in_path) {
    return boost::asio::async_initiate<CompletionToken, void(bsys::error_code)>(
        [this, l_p = std::move(in_path)](auto&& completion_handler) {
          boost::asio::post(g_thread(), [this, l_p = l_p]() { this->open(l_p); });
        },
        token
    );
  }

  // public:
  class async_open_impl {
   public:
    boost::asio::high_resolution_timer& timer_attr;
    file_translator_ptr file_translator_attr;
    state state_attr;
    FSys::path file_path;

    explicit async_open_impl(
        FSys::path in_file_path,
        boost::asio::high_resolution_timer& in_timer,
        file_translator_ptr in_file_translator
    )
        : timer_attr(in_timer),
          file_translator_attr(std::move(in_file_translator)),
          state_attr(state::init),
          file_path(std::move(in_file_path)) {}

    ~async_open_impl() {
      DOODLE_LOG_INFO("析构函数");
    }

    template <typename Self>
    void operator()(Self& self, boost::system::error_code error = {}) {
      switch (state_attr) {
        case state::init: {
          boost::asio::post(
              g_thread(),
              [&self, this]() {
                bsys::error_code l_r = file_translator_attr->open(file_path);
                if (l_r)
                  self.complete(l_r);
                state_attr = state::end;
                //                boost::asio::post(g_io_context(), std::move(self));
              }
          );
          break;
        }
        case state::end: {
          error = file_translator_attr->open_end();
          self.complete(error);
          break;
        }
      }
    }
  };

  class async_save_impl {
   public:
    boost::asio::high_resolution_timer& timer_attr;
    file_translator_ptr file_translator_attr;
    state state_attr;
    FSys::path file_path;

    explicit async_save_impl(
        FSys::path in_file_path,
        boost::asio::high_resolution_timer& in_timer,
        file_translator_ptr in_file_translator
    )
        : timer_attr(in_timer),
          file_translator_attr(std::move(in_file_translator)),
          state_attr(state::init),
          file_path(std::move(in_file_path)) {}

    template <typename Self>
    void operator()(Self& self, boost::system::error_code error = {}) {
      switch (state_attr) {
        case state::init: {
          boost::asio::post(
              g_thread(),
              [&]() {
                bsys::error_code l_r = file_translator_attr->save(file_path);
                if (l_r) self.complete(l_r);
                state_attr = state::end;
              }
          );
          break;
        }
        case state::end: {
          error = file_translator_attr->save_end();
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
    open_begin(in_path);
    //    return boost::asio::async_compose<CompletionToken,
    //                                      void(bsys::error_code)>(
    //        async_open_impl{in_path, l_time, this->shared_from_this()}, token, l_time);
    return boost::asio::async_initiate<CompletionToken, void(bsys::error_code)>(
        [l_s = this->shared_from_this(), in_path](auto&& completion_handler) {
          std::function<void(bsys::error_code)> call{completion_handler};
          boost::asio::post(g_thread(), [l_s, in_path, call]() {
            auto l_r = l_s->open(in_path);
            boost::asio::post(g_io_context(), [call, l_r, l_s]() {
              l_s->open_end();
              call(l_r);
            });
          });
        },
        token
    );
  };

  /**
   * @brief 使用路径打开项目文件
   * @param in_path 传入的项目文件路径
   */
  template <typename CompletionToken>
  auto async_save(const FSys::path& in_path, CompletionToken&& token)
      ->
      typename boost::asio::async_result<
          typename std::decay_t<CompletionToken>,
          void(bsys::error_code)>::return_type {
    save_begin(in_path);
    //    boost::asio::high_resolution_timer l_time{g_io_context()};
    //
    //    return boost::asio::async_compose<CompletionToken,
    //                                      void(bsys::error_code)>(
    //        async_save_impl{in_path, l_time, this->shared_from_this()}, token, l_time);

    return boost::asio::async_initiate<CompletionToken, void(bsys::error_code)>(
        [l_s = this->shared_from_this(), in_path](auto&& completion_handler) {
          std::function<void(bsys::error_code)> call{completion_handler};
          boost::asio::post(g_thread(), [l_s, in_path, call]() {
            auto l_r = l_s->save(in_path);
            boost::asio::post(g_io_context(), [call, l_r, l_s]() {
              l_s->save_end();
              call(l_r);
            });
          });
        },
        token
    );
  };

  virtual void new_file_scene(const FSys::path& in_path);
};

class DOODLE_CORE_API sqlite_file : public file_translator {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 protected:
  bsys::error_code open_impl(const FSys::path& in_path) override;
  bsys::error_code save_impl(const FSys::path& in_path) override;

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
