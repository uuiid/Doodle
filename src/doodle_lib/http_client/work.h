//
// Created by TD on 2024/2/29.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/process.hpp>
namespace doodle::http {
class http_websocket_data;
/**
 * @brief http_work  主要是接受任务, 并开始执行任务的类
 *
 * 任务 : maya_exe, UE_exe, auto_light_task
 *
 */

enum class run_task_type {
  maya_exe,         // 运行maya任务, 这个不会转发到 maya.exe, 会转发给 我们自己编译的exe中
  UE_exe,           // 运行UE任务, 这个会直接转发到 UE-cmd.exe 中
  auto_light_task,  // 这个是组合任务, 会自动执行一些任务
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    run_task_type,
    {
        {run_task_type::maya_exe, "maya_exe"},
        {run_task_type::UE_exe, "UE_exe"},
        {run_task_type::auto_light_task, "auto_light_task"},
    }
)

class http_work {
  //  using client_core     = doodle::detail::client_core;
  //  using client_core_ptr = std::shared_ptr<client_core>;
  //  boost::asio::basic_io_object;
  using timer           = boost::asio::high_resolution_timer;
  using timer_ptr       = std::shared_ptr<timer>;

  using response_type   = boost::beast::http::response<boost::beast::http::string_body>;
  using request_type    = boost::beast::http::request<boost::beast::http::string_body>;
  using signal_set      = boost::asio::signal_set;
  using signal_set_ptr  = std::shared_ptr<signal_set>;
  using logger_sink_ptr = std::shared_ptr<spdlog::sinks::sink>;
  class websocket_run_task_fun;
  // 自动连接定时器
  timer_ptr timer_{};

  std::shared_ptr<http_websocket_data> websocket_data_{};

  std::string server_address_{};
  std::uint16_t port_{};

  logger_ptr logger_{};
  std::string task_id_{};
  // 执行程序
  std::string exe_{};
  // 命令行
  std::vector<std::string> command_line_{};

  bool is_connect_{false};
  std::string host_name_{};
  // 子进程
  boost::process::child child_{};
  boost::asio::streambuf out_strbuff_attr{};
  boost::asio::streambuf err_strbuff_attr{};
  std::shared_ptr<boost::process::async_pipe> out_pipe_{};
  std::shared_ptr<boost::process::async_pipe> err_pipe_{};

  // async read out
  void do_read_out();
  void do_read_err();

  friend class websocket_sink_mt;

  void do_connect();

  void do_wait();

  void send_state();

  void read_task_info(const nlohmann::json& in_json, const std::shared_ptr<http_websocket_data>& in_handle);

  void run_task();

  void end_task(boost::system::error_code in_error_code);

 public:
  http_work()  = default;
  ~http_work() = default;

  void run(const std::string& in_server_address, std::uint16_t in_port = doodle_config::http_port);
};
}  // namespace doodle::http
