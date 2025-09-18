//
// Created by TD on 24-7-19.
//

#pragma once
#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/locale/encoding.hpp>

#include <memory>

namespace doodle {

template <typename ReadablePipe, typename Handle>
auto async_read_pipe(
    ReadablePipe& in_readable_pipe, logger_ptr in_logger, Handle&& in_handle, level::level_enum in_level = level::info,
    std::string in_out_code = "GBK"
) {
  for (auto&& i : in_out_code) i = std::tolower(i);
  auto l_buffer = std::make_shared<boost::asio::streambuf>();
  return boost::asio::async_compose<Handle, void(boost::system::error_code)>(
      [&in_readable_pipe, in_logger, l_buffer, in_out_code, in_level,
       coro = boost::asio::coroutine{}](auto& self, boost::system::error_code in_ec = {}, std::size_t = 0) mutable {
        BOOST_ASIO_CORO_REENTER(coro) {
          while (!in_ec) {
            BOOST_ASIO_CORO_YIELD boost::asio::async_read_until(in_readable_pipe, *l_buffer, '\n', std::move(self));
            if (in_ec) {
              if (in_ec == boost::asio::error::operation_aborted || in_ec == boost::asio::error::broken_pipe) {
                break;
              }
              in_logger->warn(in_ec.what());
              break;
            }
            std::string l_line{};
            std::istream l_is{&(*l_buffer)};
            std::getline(l_is, l_line);
            while (!l_line.empty() && std::iscntrl(l_line.back(), core_set::get_set().utf8_locale)) l_line.pop_back();
            if (!l_line.empty()) {
              l_line = boost::locale::conv::to_utf<char>(l_line, in_out_code);
              in_logger->log(in_level, l_line);
            }
          }
          self.complete(in_ec);
        }
      },
      in_handle, in_readable_pipe
  );
}
}  // namespace doodle