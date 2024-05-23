//
// Created by TD on 2024/2/29.
//

#include "work.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/auto_light_render_video.h>
#include <doodle_lib/core/down_auto_light_anim_file.h>
#include <doodle_lib/core/http/http_websocket_data.h>
#include <doodle_lib/core/http/websocket_route.h>
#include <doodle_lib/core/up_auto_light_file.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_method/computer.h>

#include "exe_warp/maya_exe.h"
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
namespace doodle::http {

class http_work::websocket_run_task_fun {
  std::string task_id_{};
  // 执行程序
  std::string exe_{};
  // 命令行
  std::vector<std::string> command_line_{};

  // 子进程
  boost::process::child child_{};
  boost::asio::streambuf out_strbuff_attr{};
  boost::asio::streambuf err_strbuff_attr{};
  std::shared_ptr<boost::process::async_pipe> out_pipe_{};
  std::shared_ptr<boost::process::async_pipe> err_pipe_{};

 public:
  http_work *http_work_;
  boost::asio::any_io_executor executor_;
  static constexpr std::string_view name{"run_task"};

  explicit websocket_run_task_fun(http_work *in_http_work)
      : http_work_(in_http_work), executor_(g_io_context().get_executor()) {}
  boost::asio::any_io_executor get_executor() const { return executor_; }

  void operator()(const std::shared_ptr<http_websocket_data> &in_handle, const nlohmann::json &in_json) {
    if (!in_json.contains("id")) {
      http_work_->logger_->log(log_loc(), level::err, "json parse error: {}", in_json.dump());
      return;
    }
    task_id_      = in_json["id"].get<std::string>();
    command_line_ = in_json["command"].get<std::vector<std::string>>();
    exe_          = in_json["exe"].get<std::string>();
    run_task();
  }

  void run_task() {
    out_pipe_      = std::make_shared<boost::process::async_pipe>(g_io_context());
    err_pipe_      = std::make_shared<boost::process::async_pipe>(g_io_context());
    auto l_run_exe = boost::process::search_path(exe_);
    child_         = boost::process::child{
        g_io_context(),
        boost::process::exe  = l_run_exe,
        boost::process::args = command_line_,
        boost::process::std_out > *out_pipe_,
        boost::process::std_err > *err_pipe_,
        boost::process::on_exit =
            [this](int exit_code, const std::error_code &ec) {
              if (ec) {
                http_work_->logger_->log(log_loc(), level::err, "run task error: {}", ec.message());
                http_work_->end_task(ec);
                return;
              }
              http_work_->logger_->log(log_loc(), level::info, "run task success: {}", exit_code);
              http_work_->end_task(ec);
            },
        boost::process::windows::hide
    };
    do_read_out();
    do_read_err();
  }

  // async read out
  void do_read_out() {
    boost::asio::async_read_until(
        *out_pipe_, out_strbuff_attr, "\n",
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(),
            [this](boost::system::error_code in_error_code, std::size_t in_size) {
              if (in_error_code) {
                http_work_->logger_->log(log_loc(), level::err, "do_read_out error: {}", in_error_code);
                return;
              }
              std::istream l_stream(&out_strbuff_attr);
              std::string l_line{};
              std::getline(l_stream, l_line);
              http_work_->websocket_data_->seed(
                  nlohmann::json{{"type", "logger"}, {"level", level::info}, {"task_id", task_id_}, {"msg", l_line}}
              );
              do_read_out();
            }
        )
    );
  }
  void do_read_err() {
    boost::asio::async_read_until(
        *err_pipe_, out_strbuff_attr, "\n",
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(),
            [this](boost::system::error_code in_error_code, std::size_t in_size) {
              if (in_error_code) {
                http_work_->logger_->log(log_loc(), level::err, "do_read_out error: {}", in_error_code);
                return;
              }
              std::istream l_stream(&out_strbuff_attr);
              std::string l_line;
              std::getline(l_stream, l_line);
              http_work_->websocket_data_->seed(
                  nlohmann::json{{"type", "logger"}, {"level", level::err}, {"task_id", task_id_}, {"msg", l_line}}
              );
              do_read_err();
            }
        )
    );
  }
};

void http_work::run(const std::string &in_server_address, std::uint16_t in_port) {
  timer_          = std::make_shared<timer>(g_io_context());
  server_address_ = in_server_address;
  port_           = in_port;

  websocket_data_ = std::make_shared<http_websocket_data>();
  logger_         = g_logger_ctrl().make_log("http_work");

  core_set_init{}.config_to_user();
  app_base::Get().on_cancel.slot().assign([this](boost::asio::cancellation_type_t in_error_code) {
    websocket_data_->do_close();
  });
  host_name_       = boost::asio::ip::host_name();

  auto l_web_route = std::make_shared<websocket_route>();
  l_web_route->reg(websocket_run_task_fun{this});
  websocket_data_->route_ptr_ = l_web_route;
  do_connect();
}
void http_work::do_connect() {
  websocket_data_->async_connect(
      server_address_, "v1/computer", port_,
      boost::asio::bind_cancellation_slot(
          app_base::Get().on_cancel.slot(),
          [this](boost::system::error_code in_error_code) {
            if (in_error_code) {
              logger_->log(log_loc(), level::err, "连接失败 {}", in_error_code);
              do_wait();
              return;
            }
            is_connect_ = true;
            send_state();
          }
      )
  );
}
void http_work::do_wait() {
  // logger_->log(log_loc(), level::info, "开始等待下一次心跳");
  timer_->expires_after(std::chrono::seconds{2});
  timer_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::Get().on_cancel.slot(),
      [this](boost::system::error_code in_error_code) {
        if (in_error_code == boost::asio::error::operation_aborted) {
          return;
        }
        if (in_error_code) {
          logger_->log(log_loc(), level::err, "on_wait error: {}", in_error_code);
          return;
        }
        if (!is_connect_) {
          do_connect();
          return;
        }
        if(!websocket_data_->stream_->is_open()) {
          do_connect();
          return;
        }
        do_wait();
      }
  ));
}

void http_work::send_state() {
  if (task_id_.empty()) {
    websocket_data_->seed(
        nlohmann::json{{"type", "set_state"}, {"state", computer_status::free}, {"host_name", host_name_}}
    );
  } else {
    websocket_data_->seed(
        nlohmann::json{{"type", "set_state"}, {"state", computer_status::busy}, {"host_name", host_name_}}
    );
  }
  do_wait();
}

void http_work::end_task(boost::system::error_code in_error_code) {
  if (in_error_code) {
    logger_->log(log_loc(), level::err, "任务执行失败: {}", in_error_code);
  } else {
    logger_->log(log_loc(), level::info, "任务执行成功");
  }
  websocket_data_->seed(nlohmann::json{
      {"type", computer_websocket_fun::set_task},
      {"status", in_error_code ? server_task_info_status::failed : server_task_info_status::completed}
  });
  task_id_ = {};
  send_state();
}

}  // namespace doodle::http