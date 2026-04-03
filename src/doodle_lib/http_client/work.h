//
// Created by TD on 2024/2/29.
//

#pragma once
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include "doodle_lib/http_client/kitsu_client.h"
#include <doodle_lib/core/http/http_websocket_client.h>
#include <doodle_lib/core/http_client_core.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/lockfree/spsc_value.hpp>
#include <boost/process.hpp>

#include <memory>
#include <optional>
#include <string>

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
  boost::asio::strand<boost::asio::io_context::executor_type> strand_{boost::asio::make_strand(g_io_context())};
  boost::asio::awaitable<void> async_run();
  computer this_computer_info_;
  bool is_writing_{false};
  boost::lockfree::spsc_queue<std::string, boost::lockfree::capacity<1024>> message_queue_;
  boost::lockfree::spsc_value<boost::beast::websocket::ping_data> ping_message_;
  boost::asio::cancellation_signal on_cancel_;
  boost::asio::awaitable<void> async_write_msg();

 protected:
  bool run_task(const server_task_info& in_task_info);

  void begin_write_msg();
  void begin_ping();
  boost::asio::awaitable<void> async_ping_loop();

 public:
  http_work()  = default;
  ~http_work() = default;

  void run(const std::string& in_token);
  void set_computer_status(computer_status in_status);
};

class base_distributed_task {
 protected:
  server_task_info task_info_;
  std::shared_ptr<http_work> http_work_ptr_;

 public:
  explicit base_distributed_task(server_task_info in_task_info, std::shared_ptr<http_work> in_http_work_ptr)
      : task_info_(std::move(in_task_info)), http_work_ptr_(std::move(in_http_work_ptr)) {}
  virtual ~base_distributed_task();

  logger_ptr create_logger() const;

  std::shared_ptr<kitsu::kitsu_client> create_kitsu_client() const;
};

}  // namespace doodle::http
