//
// Created by TD on 2024/1/6.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>
#include <boost/asio/prepend.hpp>
namespace doodle {

class thread_copy_io_service {
  using copy_fun_ptr = void (*)(const FSys::path&, const FSys::path&);
  static void copy_file(const FSys::path& from, const FSys::path& to);
  static void copy_old_file(const FSys::path& from, const FSys::path& to);
  boost::system::error_code copy_impl(
      const FSys::path& from, const FSys::path& to, FSys::copy_options in_options, logger_ptr in_logger,
      copy_fun_ptr in_fun_ptr = copy_file
  ) const;

  boost::system::error_code delete_impl(
      const std::vector<FSys::path>& from, const FSys::path& to, const std::vector<FSys::path>& in_exclude_local_dir,
      FSys::copy_options in_options, logger_ptr in_logger
  ) const;

  decltype(boost::asio::make_strand(g_thread())) executor_{boost::asio::make_strand(g_thread())};

 public:
  thread_copy_io_service() {}
  ~thread_copy_io_service() = default;

  /**
   * @brief 这个是差异复制, 会自动比较文件的修改时间, 和大小 如果源文件的修改时间或者大小不同, 则会复制, 否则不会复制
   * @tparam CompletionHandler
   * @param from 源文件
   * @param to 目标文件
   * @param in_options 复制选项
   * @param handler 完成回调
   * @param in_logger 日志
   * @return
   */
  template <typename CompletionHandler>
  auto async_copy(
      const FSys::path& from, const FSys::path& to, FSys::copy_options in_options, logger_ptr in_logger,
      CompletionHandler&& handler
  ) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code)>(
        [this](
            auto&& handler, const FSys::path& from, const FSys::path& to, FSys::copy_options in_options,
            logger_ptr in_logger
        ) {
          auto l_handler = std::make_shared<std::decay_t<decltype(handler)>>(std::forward<decltype(handler)>(handler));
          boost::asio::post(
              executor_,
              [this, l_handler, from, to, in_options, in_logger,
               l_gruad = boost::asio::make_work_guard(boost::asio::get_associated_executor(*l_handler))]() {
                auto l_ec = this->copy_impl(from, to, in_options, in_logger, copy_file);
                boost::asio::post(boost::asio::prepend(*l_handler, l_ec));
              }
          );
        },
        handler, from, to, in_options, in_logger
    );
  }

  template <typename CompletionHandler>
  auto async_copy_old(
      const FSys::path& from, const FSys::path& to, FSys::copy_options in_options, logger_ptr in_logger,
      CompletionHandler&& handler
  ) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code)>(
        [this](
            auto&& handler, const FSys::path& from, const FSys::path& to, FSys::copy_options in_options,
            logger_ptr in_logger
        ) {
          auto l_handler = std::make_shared<std::decay_t<decltype(handler)>>(std::forward<decltype(handler)>(handler));
          boost::asio::post(
              executor_,
              [this, l_handler, from, to, in_options, in_logger,
               l_gruad = boost::asio::make_work_guard(boost::asio::get_associated_executor(*l_handler))]() {
                auto l_ec = this->copy_impl(from, to, in_options, in_logger, copy_old_file);
                boost::asio::post(boost::asio::prepend(*l_handler, l_ec));
              }
          );
        },
        handler, from, to, in_options, in_logger
    );
  }

  template <typename CompletionHandler>
  auto async_copy(
      const std::vector<std::pair<FSys::path, FSys::path>>& from_and_to, FSys::copy_options in_options,
      logger_ptr in_logger, CompletionHandler&& handler
  ) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code)>(
        [this](
            auto&& handler, const std::vector<std::pair<FSys::path, FSys::path>>& from_and_to,
            FSys::copy_options in_options, logger_ptr in_logger
        ) {
          auto l_handler = std::make_shared<std::decay_t<decltype(handler)>>(std::forward<decltype(handler)>(handler));
          boost::asio::post(
              executor_,
              [this, l_handler, from_and_to, in_options, in_logger,
               l_gruad = boost::asio::make_work_guard(boost::asio::get_associated_executor(*l_handler))]() {
                boost::system::error_code l_ec;
                for (auto&& [from, to] : from_and_to) {
                  l_ec = this->copy_impl(from, to, in_options, in_logger, copy_file);
                  if (l_ec) {
                    boost::asio::post(boost::asio::prepend(*l_handler, l_ec));
                    return;
                  }
                }
                boost::asio::post(boost::asio::prepend(*l_handler, l_ec));
              }
          );
        },
        handler, from_and_to, in_options, in_logger
    );
  }

  template <typename CompletionHandler>
  auto async_copy_old(
      const std::vector<std::pair<FSys::path, FSys::path>>& from_and_to, FSys::copy_options in_options,
      logger_ptr in_logger, CompletionHandler&& handler
  ) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code)>(
        [this](
            auto&& handler, const std::vector<std::pair<FSys::path, FSys::path>>& from_and_to,
            FSys::copy_options in_options, logger_ptr in_logger
        ) {
          auto l_handler = std::make_shared<std::decay_t<decltype(handler)>>(std::forward<decltype(handler)>(handler));
          boost::asio::post(
              executor_,
              [this, l_handler, from_and_to, in_options, in_logger,
               l_gruad = boost::asio::make_work_guard(boost::asio::get_associated_executor(*l_handler))]() {
                boost::system::error_code l_ec;
                for (auto&& [from, to] : from_and_to) {
                  l_ec = this->copy_impl(from, to, in_options, in_logger, copy_old_file);
                  if (l_ec) {
                    boost::asio::post(boost::asio::prepend(*l_handler, l_ec));
                    return;
                  }
                }
                boost::asio::post(boost::asio::prepend(*l_handler, l_ec));
              }
          );
        },
        handler, from_and_to, in_options, in_logger
    );
  }
  /**
   * @brief 这个是差异复制, 会自动比较文件的修改时间, 和大小 如果源文件的修改时间或者大小不同, 则会复制, 否则不会复制,
   * 同时, 如果远程文件不存在, 则会删除本地文件
   *
   *
   * @tparam CompletionHandler  完成回调类型
   * @param from_and_to  源文件和目标文件
   * @param in_exclude_local_dir  排除的本地目录(相对路径)
   * @param in_options  复制选项
   * @param in_logger  日志
   * @param handler  完成回调
   */
  template <typename CompletionHandler>
  auto async_delete_remote_not_exit_and_copy_old(
      const std::vector<std::pair<FSys::path, FSys::path>>& from_and_to,
      const std::vector<FSys::path>& in_exclude_local_dir, FSys::copy_options in_options, logger_ptr in_logger,
      CompletionHandler&& handler
  ) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code)>(
        [this](
            auto&& handler, const std::vector<std::pair<FSys::path, FSys::path>>& from_and_to,
            const std::vector<FSys::path>& in_exclude_local_dir, FSys::copy_options in_options, logger_ptr in_logger
        ) {
          auto l_handler = std::make_shared<std::decay_t<decltype(handler)>>(std::forward<decltype(handler)>(handler));
          boost::asio::post(
              executor_,
              [this, l_handler, from_and_to, in_options, in_logger, in_exclude_local_dir,
               l_gruad = boost::asio::make_work_guard(boost::asio::get_associated_executor(*l_handler))]() {
                boost::system::error_code l_ec;

                std::vector<FSys::path> l_form_tmp{};
                std::vector<FSys::path> l_to_tmp{};
                for (auto&& [f, t] : from_and_to) {
                  l_form_tmp.emplace_back(f);
                  l_to_tmp.emplace_back(t);
                }
                // sort to_tmp
                std::sort(l_to_tmp.begin(), l_to_tmp.end(), [](const FSys::path& l, const FSys::path& r) {
                  return l > r;
                });
                // unique to_tmp
                l_to_tmp.erase(
                    std::unique(
                        l_to_tmp.begin(), l_to_tmp.end(),
                        [](const FSys::path& l, const FSys::path& r) { return l == r; }
                    ),
                    l_to_tmp.end()
                );

                // 先复制文件
                for (auto&& [from, to] : from_and_to) {
                  l_ec = this->copy_impl(from, to, in_options, in_logger, copy_old_file);
                  if (l_ec) {
                    boost::asio::post(boost::asio::prepend(*l_handler, l_ec));
                    return;
                  }
                }
                // 排除旧的文件
                for (auto&& l_to : l_to_tmp) {
                  l_ec = this->delete_impl(l_form_tmp, l_to, in_exclude_local_dir, in_options, in_logger);
                  if (l_ec) {
                    boost::asio::post(boost::asio::prepend(*l_handler, l_ec));
                    return;
                  }
                }
                boost::asio::post(boost::asio::prepend(*l_handler, l_ec));
              }
          );
        },
        handler, from_and_to, in_exclude_local_dir, in_options, in_logger
    );
  }
};


}  // namespace doodle
