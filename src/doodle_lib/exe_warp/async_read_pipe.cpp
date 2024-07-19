//
// Created by TD on 24-7-19.
//

#include "async_read_pipe.h"
#include <doodle_core/core/core_set.h>

namespace doodle {
boost::asio::awaitable<void> async_read_pipe(std::shared_ptr<boost::asio::readable_pipe> in_pip, logger_ptr in_logger,
                                             level::level_enum in_level,
                                             std::string in_out_code) {
  boost::asio::streambuf l_buffer{};
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
    auto [l_ec, l_size] = co_await
        boost::asio::async_read_until(
          *in_pip, l_buffer, '\n', boost::asio::as_tuple(boost::asio::use_awaitable));

    if (l_ec) {
      in_logger->warn(l_ec.what());
      co_return;
    }

    std::string l_line{};
    std::istream l_is{&l_buffer};
    std::getline(l_is, l_line);
    while (!l_line.empty() && std::iscntrl(l_line.back(), core_set::get_set().utf8_locale)) l_line.pop_back();
    if (!l_line.empty()) {
      in_logger->log(in_level, boost::locale::conv::to_utf<char>(l_line, in_out_code));
    }
  }
}
}