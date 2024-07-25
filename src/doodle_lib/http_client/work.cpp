//
// Created by TD on 2024/2/29.
//

#include "work.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

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
class http_work::websocket_run_task_fun : public std::enable_shared_from_this<websocket_run_task_fun> {
  struct impl {
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
    http_work* http_work_;
  };

  std::shared_ptr<impl> impl_{};

 public:
  static constexpr std::string_view name{"run_task"};

  explicit websocket_run_task_fun(http_work* in_http_work) : impl_{std::make_shared<impl>()} {
    impl_->http_work_ = in_http_work;
  }

  // copy
  websocket_run_task_fun(const websocket_run_task_fun&)            = default;
  websocket_run_task_fun& operator=(const websocket_run_task_fun&) = default;
  // move
  websocket_run_task_fun(websocket_run_task_fun&&)                 = default;
  websocket_run_task_fun& operator=(websocket_run_task_fun&&)      = default;

  void run(const std::string& in_id, const std::string& in_exe, const std::vector<std::string>& in_command_line) {
    impl_->task_id_      = in_id;
    impl_->command_line_ = in_command_line;
    impl_->exe_          = in_exe;
    run_task();
  }

  void run_task() {
    impl_->out_pipe_ = std::make_shared<boost::process::async_pipe>(g_io_context());
    impl_->err_pipe_ = std::make_shared<boost::process::async_pipe>(g_io_context());
    auto l_run_exe   = boost::process::search_path(impl_->exe_);
    impl_->http_work_->logger_->log(
        log_loc(), level::info, "run task {}: {} {}", impl_->task_id_, l_run_exe, impl_->command_line_
    );

    do_read_out();
    do_read_err();
  }

  level::level_enum log_level(const std::string& in_msg, level::level_enum in_def) const {
    level::level_enum l_level = in_def;
    if (in_msg.size() > 2 && in_msg.front() == '[') {
      if (in_msg[1] == 'i') {
        l_level = level::info;
      } else if (in_msg[1] == 'w') {
        l_level = level::warn;
      } else if (in_msg[1] == 'e') {
        l_level = level::err;
      } else if (in_msg[1] == 'c') {
        l_level = level::critical;
      } else if (in_msg[1] == 't') {
        l_level = level::trace;
      } else if (in_msg[1] == 'd') {
        l_level = level::debug;
      }
    }
    return l_level;
  }

  // async read out
  void do_read_out() {
    boost::asio::async_read_until(
        *impl_->out_pipe_, impl_->out_strbuff_attr, '\n',
        // boost::asio::bind_cancellation_slot(
        // app_base::Get().on_cancel.slot(),
        [this, l_self = shared_from_this()](boost::system::error_code in_error_code, std::size_t in_size) {
          if (in_error_code) {
            impl_->http_work_->logger_->log(log_loc(), level::err, "do_read_out error: {}", in_error_code);
            return;
          }

          std::istream l_stream(&impl_->out_strbuff_attr);
          std::string l_line{};
          std::getline(l_stream, l_line);
          auto l_level = log_level(l_line, level::info);
          // impl_->http_work_->websocket_data_->seed(
          //   nlohmann::json{{"type", "logger"}, {"level", l_level}, {"task_id", impl_->task_id_}, {"msg", l_line}}
          // );
          do_read_out();
        }
        // )
    );
  }

  void do_read_err() {
    boost::asio::async_read_until(
        *impl_->err_pipe_, impl_->err_strbuff_attr, '\n',
        // boost::asio::bind_cancellation_slot(
        // app_base::Get().on_cancel.slot(),
        [this, l_self = shared_from_this()](boost::system::error_code in_error_code, std::size_t in_size) {
          if (in_error_code) {
            impl_->http_work_->logger_->log(log_loc(), level::err, "do_read_out error: {}", in_error_code);
            return;
          }
          // if (impl_->err_strbuff_attr.size() == 0) return;
          std::istream l_stream(&impl_->err_strbuff_attr);
          std::string l_line;
          std::getline(l_stream, l_line);
          auto l_level = log_level(l_line, level::err);
          // impl_->http_work_->websocket_data_->seed(
          //   nlohmann::json{{"type", "logger"}, {"level", l_level}, {"task_id", impl_->task_id_}, {"msg", l_line}}
          // );
          do_read_err();
        }
        // )
    );
  }
};

boost::asio::awaitable<std::string> websocket_run_task_fun_launch(
    http_work* in_work, http_websocket_data_ptr in_handle
) {
  if (!in_handle->body_.contains("id")) {
    in_handle->logger_->log(log_loc(), level::err, "json parse error: {}", in_handle->body_.dump());
    co_return std::string{};
  }
  if (in_work->status_ == computer_status::busy) {
    in_handle->logger_->log(log_loc(), level::err, "computer busy: {}", in_handle->body_.dump());
    co_return std::string{};
  }

  boost::asio::co_spawn(
      in_work->executor_,
      in_work->async_run_task(
          in_handle->body_["id"].get<std::string>(), in_handle->body_["exe"].get<std::string>(),
          in_handle->body_["command"].get<std::vector<std::string>>()
      ),
      boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
  );
}

void http_work::run(const std::string& in_url) {
  executor_         = boost::asio::make_strand(g_io_context());
  timer_            = std::make_shared<timer>(executor_);
  url_              = in_url;

  websocket_client_ = std::make_shared<http_websocket_client>();
  logger_           = g_logger_ctrl().make_log("http_work");
  core_set_init{}.config_to_user();
  host_name_ = boost::asio::ip::host_name();
  boost::asio::co_spawn(
      executor_, async_run(),
      boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
  );
}

boost::asio::awaitable<void> http_work::async_run() {
  auto l_web_route = std::make_shared<websocket_route>();
  l_web_route->reg(
      "run_task", std::make_shared<websocket_route::call_fun_type>(std::bind_front(websocket_run_task_fun_launch, this))
  );
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
    co_await websocket_client_->init(url_, l_web_route);

    if (auto l_ec_1 = co_await websocket_client_->async_write_websocket(
            nlohmann::json{{"type", "set_state"}, {"state", status_.load()}, {"host_name", host_name_}}.dump()
        );
        l_ec_1) {
      logger_->error("写入错误 {}", l_ec_1);
      break;
    }
    boost::asio::co_spawn(
        executor_, websocket_client_->async_read_websocket(),
        boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
    );

    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
      timer_->expires_after(std::chrono::seconds{2});
      if (auto [l_tec] = co_await timer_->async_wait(); l_tec) {
        logger_->error("定时器错误 {}", l_tec);
        break;
      }
      if (auto l_ec = co_await websocket_client_->async_write_websocket(
              nlohmann::json{{"type", "set_state"}, {"state", status_.load()}, {"host_name", host_name_}}.dump()
          );
          l_ec) {
        logger_->error("写入错误 {}", l_ec);
        break;
      }
    }
  }
}
boost::asio::awaitable<void> http_work::async_run_task(
    std::string in_id, std::string in_exe, std::vector<std::string> in_command
) {
  status_ = computer_status::busy;
  if (auto l_ec = co_await websocket_client_->async_write_websocket(
          nlohmann::json{{"type", "set_state"}, {"state", status_.load()}, {"host_name", host_name_}}.dump()
      );
      l_ec) {
    logger_->error("写入错误 {}", l_ec);
    co_return;
  }
}

}  // namespace doodle::http