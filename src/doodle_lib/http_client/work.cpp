//
// Created by TD on 2024/2/29.
//

#include "work.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/core/http/websocket_route.h>
#include <doodle_lib/core/up_auto_light_file.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/exe_warp/windows_hide.h>
#include <doodle_lib/http_method/computer.h>

#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/process/v2.hpp>

#include <spdlog/sinks/basic_file_sink.h>

namespace doodle::http {
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
  task_id_ = in_id;
  status_  = computer_status::busy;
  if (auto l_ec = co_await websocket_client_->async_write_websocket(
          nlohmann::json{{"type", "set_state"}, {"state", status_.load()}, {"host_name", host_name_}}.dump()
      );
      l_ec) {
    logger_->error("写入错误 {}", l_ec);
    co_return;
  }

  auto l_timer        = std::make_shared<boost::asio::high_resolution_timer>(g_io_context());
  auto l_out_pipe     = std::make_shared<boost::asio::readable_pipe>(g_io_context());
  auto l_err_pipe     = std::make_shared<boost::asio::readable_pipe>(g_io_context());
  auto l_process_maya = boost::process::v2::process{
      g_io_context(), in_exe, in_command, boost::process::v2::process_stdio{nullptr, *l_out_pipe, *l_err_pipe},
      details::hide_and_not_create_windows
  };
  boost::asio::co_spawn(executor_, async_read_pip(l_out_pipe), boost::asio::detached);
  boost::asio::co_spawn(executor_, async_read_pip(l_err_pipe), boost::asio::detached);
  l_timer->expires_after(chrono::seconds{core_set::get_set().timeout});
  auto [l_array_completion_order, l_ec, l_exit_code, l_ec_t] =
      co_await boost::asio::experimental::make_parallel_group(
          boost::process::v2::async_execute(std::move(l_process_maya), boost::asio::deferred),
          l_timer->async_wait(boost::asio::deferred)
      )
          .async_wait(boost::asio::experimental::wait_for_one(), boost::asio::as_tuple(boost::asio::use_awaitable));

  switch (l_array_completion_order[0]) {
    case 0:
      if (l_exit_code != 0 || l_ec) {
        if (!l_ec) l_ec = {l_exit_code, exit_code_category::get()};
        logger_->error("进程返回值错误 {}", l_exit_code);
      }
      co_return;
    case 1:
      if (l_ec) {
        logger_->error("maya 运行超时: {}", l_ec.message());
      }
    default:
      co_return;
  }
}
level::level_enum log_level(const std::string& in_msg, level::level_enum in_def = level::info) {
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
boost::asio::awaitable<void> http_work::async_read_pip(std::shared_ptr<boost::asio::readable_pipe> in_pipe) {
  boost::asio::streambuf l_buffer{};
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    auto [l_ec, l_size] = co_await boost::asio::async_read_until(
        *in_pipe, l_buffer, '\n', boost::asio::as_tuple(boost::asio::use_awaitable)
    );

    if (l_ec) {
      if (l_ec == boost::asio::error::operation_aborted || l_ec == boost::asio::error::broken_pipe) {
        co_return;
      }
      logger_->warn(l_ec.what());
      co_return;
    }

    std::string l_line{};
    std::istream l_is{&l_buffer};
    std::getline(l_is, l_line);
    while (!l_line.empty() && std::iscntrl(l_line.back(), core_set::get_set().utf8_locale)) l_line.pop_back();
    if (!l_line.empty()) {
      co_await websocket_client_->async_write_websocket(nlohmann::json{
          {"type", "logger"}, {"level", log_level(l_line)}, {"task_id", task_id_}, {"msg", l_line}
      }.dump());
    }
  }
}

}  // namespace doodle::http