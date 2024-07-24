//
// Created by td_main on 2023/7/26.
//

#pragma once

#include <doodle_core/core/co_queue.h>
#include <doodle_core/doodle_core_fwd.h>
#include <boost/asio.hpp>
#include <boost/system.hpp>
#include <memory>


namespace doodle {
namespace ue_exe_ns {
std::string get_file_version(const FSys::path& in_path);
}

class ue_ctx {
public:
  ue_ctx()  = default;
  ~ue_ctx() = default;
  std::shared_ptr<awaitable_queue_limitation> queue_ = std::make_shared<awaitable_queue_limitation>();
};

boost::asio::awaitable<boost::system::error_code> async_run_ue(const std::vector<std::string>& in_arg, logger_ptr in_logger);
} // namespace doodle