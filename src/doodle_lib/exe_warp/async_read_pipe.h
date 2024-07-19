//
// Created by TD on 24-7-19.
//

#pragma once
#include <boost/asio.hpp>
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
boost::asio::awaitable<void> async_read_pipe(std::shared_ptr<boost::asio::readable_pipe> in_pip, logger_ptr in_logger,
                                             level::level_enum in_level = level::info, std::string in_out_code = "GBK");
}