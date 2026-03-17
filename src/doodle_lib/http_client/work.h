//
// Created by TD on 2024/2/29.
//

#pragma once
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_websocket_client.h>
#include <doodle_lib/core/http_client_core.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/process.hpp>

namespace doodle::http {
namespace detail {
class http_websocket_data;
}

using http_websocket_data_ptr = std::shared_ptr<detail::http_websocket_data>;
/**
 * @brief http_work  主要是接受任务, 并开始执行任务的类
 *
 * 任务 : maya_exe, UE_exe, auto_light_task
 *
 */

class http_work : public std::enable_shared_from_this<http_work> {
  using timer     = boost::asio::high_resolution_timer;
  using timer_ptr = std::shared_ptr<timer>;

  boost::asio::any_io_executor executor_{};
  logger_ptr logger_{};
  std::string token_{};
  std::shared_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>> websocket_client_{};
  boost::asio::awaitable<void> async_run();
  computer this_computer_info_;

 protected:
  void run_task(const server_task_info& in_task_info);

 public:
  http_work()  = default;
  ~http_work() = default;

  void run(const std::string& in_token);
  boost::asio::awaitable<void> set_computer_status(computer_status in_status);
};
}  // namespace doodle::http
