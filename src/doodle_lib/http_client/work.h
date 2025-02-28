//
// Created by TD on 2024/2/29.
//

#pragma once
#include <doodle_core/core/http_client_core.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_websocket_client.h>

#include <boost/asio.hpp>
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

class http_work {
  using executor_type = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using timer         = executor_type::as_default_on_t<boost::asio::high_resolution_timer>;
  using timer_ptr     = std::shared_ptr<timer>;

  boost::asio::awaitable<std::string> websocket_run_task_fun_launch(http_websocket_data_ptr in_handle);
  boost::asio::awaitable<std::string> websocket_list_task_fun_launch(http_websocket_data_ptr in_handle);
  boost::asio::any_io_executor executor_{};
  // 自动连接定时器
  timer_ptr timer_{};

  std::shared_ptr<http_websocket_client> websocket_client_{};
  detail::http_client_data_base_ptr client_data_{};

  logger_ptr logger_{};
  computer_status status_{computer_status::online};

  uuid task_id_{};

  std::string host_name_{};
  std::string url_{};
  uuid uuid_id_{};
  std::vector<uuid> run_task_ids_{};

  struct run_task_info {
    std::string run_exe{};
    std::vector<std::string> run_args{};
    server_task_info_status status{};
  };

  boost::asio::awaitable<void> async_run();

  boost::asio::awaitable<tl::expected<run_task_info, std::string>> get_task_data();

  template <typename Handle>
  auto async_relay_websocket(std::shared_ptr<boost::asio::readable_pipe> in_pipe, Handle&& in_handle);

  boost::asio::awaitable<void> async_set_status(computer_status in_status);
  boost::asio::awaitable<void> async_set_task_status(server_task_info_status in_status);
  boost::asio::awaitable<void> async_read_pip(std::shared_ptr<boost::asio::readable_pipe> in_pipe);

 protected:
  virtual boost::asio::awaitable<void> async_run_task();

 public:
  http_work()  = default;
  ~http_work() = default;

  void run(const std::string& in_web_socket_url, const std::string& in_http_url, const uuid& in_uuid = uuid{});
};
}  // namespace doodle::http
