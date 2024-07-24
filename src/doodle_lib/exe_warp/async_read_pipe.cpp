//
// Created by TD on 24-7-19.
//

#include "async_read_pipe.h"
#include <doodle_core/core/core_set.h>
#include "boost/locale/encoding.hpp"

namespace doodle {
boost::asio::awaitable<void> async_read_pipe(std::shared_ptr<boost::asio::readable_pipe> in_pip, logger_ptr in_logger,
                                             level::level_enum in_level,
                                             std::string in_out_code) {
  for (auto&& i : in_out_code)
    std::tolower(i);
  boost::asio::streambuf l_buffer{};
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    auto [l_ec, l_size] = co_await
        boost::asio::async_read_until(
          *in_pip, l_buffer, '\n', boost::asio::as_tuple(boost::asio::use_awaitable));

    if (l_ec) {
      if (l_ec == boost::asio::error::operation_aborted || l_ec == boost::asio::error::broken_pipe) {
        co_return;
      }
      in_logger->warn(l_ec.what());
      co_return;
    }

    std::string l_line{};
    std::istream l_is{&l_buffer};
    std::getline(l_is, l_line);
    while (!l_line.empty() && std::iscntrl(l_line.back(), core_set::get_set().utf8_locale)) l_line.pop_back();
    if (!l_line.empty()) {
      if (in_out_code != "utf-8")
        l_line = boost::locale::conv::to_utf<char>(l_line, in_out_code);
      in_logger->log(in_level, l_line);
    }
  }
}
}