//
// Created by td_main on 2023/7/26.
//

#pragma once

#include <doodle_core/core/co_queue.h>

#include <doodle_core/core/global_function.h>
#include <doodle_core/core/wait_op.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>
#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/process.hpp>
#include <boost/process/windows.hpp>
#include <boost/system.hpp>

#include <cstdint>
#include <memory>
#include <stack>
#include <utility>
#include <vector>

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

boost::asio::awaitable<boost::system::error_code> async_run_ue(const std::string& in_arg, logger_ptr in_logger);
} // namespace doodle