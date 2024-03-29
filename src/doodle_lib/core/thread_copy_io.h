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
  decltype(boost::asio::make_strand(g_thread())) executor_{boost::asio::make_strand(g_thread())};

 public:
  thread_copy_io_service() {}
  ~thread_copy_io_service() = default;

  /**
   * @brief 这个是差异复制, 会自动比较文件的修改时间, 和大小 如果源文件的修改时间或者大小不同, 则会复制, 否则不会复制
   * @tparam CompletionHandler
   * @param from
   * @param to
   * @param in_options
   * @param handler
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
};

}  // namespace doodle
