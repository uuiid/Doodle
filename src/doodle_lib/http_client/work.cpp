//
// Created by TD on 2024/2/29.
//

#include "work.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/websocket_route.h>
#include <doodle_lib/exe_warp/windows_hide.h>
#include <doodle_lib/http_method/computer.h>

#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/process/process.hpp>

#include "core/http/json_body.h"
#include <spdlog/sinks/basic_file_sink.h>

namespace doodle::http {

boost::asio::awaitable<std::string> http_work::websocket_run_task_fun_launch(http_websocket_data_ptr in_handle) {
  try {
    DOODLE_TO_EXECUTOR(executor_);
    run_task_ids_.emplace_back(in_handle->body_["id"].get<uuid>());
  } catch (...) {
    default_logger_raw()->error("worker run task error {}", boost::current_exception_diagnostic_information());
  }

  co_return std::string{};
}
boost::asio::awaitable<std::string> http_work::websocket_list_task_fun_launch(http_websocket_data_ptr in_handle) {
  try {
    DOODLE_TO_EXECUTOR(executor_);
    run_task_ids_ = in_handle->body_["ids"].get<std::vector<uuid>>();
  } catch (...) {
    default_logger_raw()->error("worker list task error {}", boost::current_exception_diagnostic_information());
  }
  co_return std::string{};
}

boost::asio::awaitable<tl::expected<http_work::run_task_info, std::string>> http_work::get_task_data() {
  run_task_info l_info{};

  co_return tl::expected<http_work::run_task_info, std::string>{l_info};
}

void http_work::run(const std::string& in_url, const std::string& in_http_url, const uuid& in_uuid) {
  executor_ = boost::asio::make_strand(g_io_context());
  timer_    = std::make_shared<timer>(executor_);
  url_      = in_url;
  uuid_id_  = in_uuid;
  if (uuid_id_.is_nil()) uuid_id_ = core_set::get_set().user_id;

  websocket_client_ = std::make_shared<http_websocket_client>();
  logger_           = g_logger_ctrl().make_log("http_work");
  host_name_        = boost::asio::ip::host_name();
  boost::asio::co_spawn(
      executor_, async_run(),
      boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
  );
}

boost::asio::awaitable<void> http_work::async_run() {
  auto l_web_route = std::make_shared<websocket_route>();
  l_web_route->reg(
      std::string{doodle_config::work_websocket_event::post_task},
      websocket_route::call_fun_type{std::bind_front(&http_work::websocket_run_task_fun_launch, this)}
  );
  l_web_route->reg(
      std::string{doodle_config::work_websocket_event::list_task},
      websocket_route::call_fun_type{std::bind_front(&http_work::websocket_list_task_fun_launch, this)}

  );
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    if (auto l_ec = co_await websocket_client_->init(url_, l_web_route); l_ec) {
      continue;
    }
    status_ = computer_status::online;
    if (auto l_e = co_await websocket_client_->async_ping(); !l_e) {
      logger_->error(l_e.error());
      continue;
    }
    co_await async_set_status(status_);
    boost::asio::co_spawn(
        executor_, websocket_client_->async_read_websocket(),
        boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
    );
    status_ = computer_status::free;
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      timer_->expires_after(std::chrono::seconds{2});
      if (auto [l_tec] = co_await timer_->async_wait(); l_tec) {
        logger_->error("定时器错误 {}", l_tec);
        break;
      }
      if (auto l_e = co_await websocket_client_->async_ping(); !l_e) {
        logger_->error(l_e.error());
        break;
      }
      if (status_ == computer_status::free && !run_task_ids_.empty()) {
        status_ = computer_status::busy;
        boost::asio::co_spawn(
            executor_, async_run_task(),
            boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
        );
      }
      co_await async_set_status(status_);
    }
  }
}
boost::asio::awaitable<void> http_work::async_run_task() {
  if (auto l_e = co_await websocket_client_->async_ping(); !l_e) co_return logger_->error(l_e.error());

  task_id_ = run_task_ids_.front();
  std::ranges::rotate(run_task_ids_, run_task_ids_.begin() + 1);
  run_task_ids_.pop_back();

  auto l_task_info = co_await get_task_data();
  if (!l_task_info) {
    co_await async_set_task_status(server_task_info_status::failed);
    logger_->error("获取任务数据失败 {}", l_task_info.error());
    co_return;
  }
  if (l_task_info->status == server_task_info_status::canceled) {
    logger_->warn("任务已取消 {}", task_id_);
    co_return;
  }
  co_await async_set_task_status(server_task_info_status::running);
  auto l_timer      = std::make_shared<boost::asio::high_resolution_timer>(g_io_context());
  auto l_out_pipe   = std::make_shared<boost::asio::readable_pipe>(g_io_context());
  auto l_err_pipe   = std::make_shared<boost::asio::readable_pipe>(g_io_context());
  FSys::path l_path = l_task_info->run_exe;
  if (!l_path.has_root_name()) l_path = boost::process::v2::environment::find_executable(l_task_info->run_exe);
  auto l_process_maya = boost::process::v2::process{
      g_io_context(), l_path, l_task_info->run_args,
      boost::process::v2::process_stdio{nullptr, *l_out_pipe, *l_err_pipe}, details::hide_and_not_create_windows
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
    case 0: {
      if (l_exit_code != 0 || l_ec) {
        logger_->error("进程返回值错误 {}", l_exit_code);
      }
      co_await async_set_task_status(
          l_exit_code == 0 ? server_task_info_status::completed : server_task_info_status::failed
      );
    } break;
    case 1:
      if (l_ec) {
        logger_->error("运行超时: {}", l_ec.message());
      }
      co_await async_set_task_status(server_task_info_status::failed);
      break;
    default:
      break;
  }
  status_ = computer_status::free;
  co_return;
}

// template <typename Handle>
// auto http_work::async_relay_websocket(std::shared_ptr<boost::asio::readable_pipe> in_pipe, Handle&& in_handle) {
//   auto l_flat_buffer = std::make_shared<boost::asio::streambuf>();
//   return boost::asio::async_compose<Handle, void(boost::system::error_code)>(
//       [l_flat_buffer, in_pipe, this,
//        coro = boost::asio::coroutine{}](auto& self, boost::system::error_code in_ec = {}, std::size_t = 0) mutable {
//         BOOST_ASIO_CORO_REENTER(coro) {
//           while (!in_ec) {
//             BOOST_ASIO_CORO_YIELD boost::asio::async_read_until(*in_pipe, l_flat_buffer, '\n', std::move(self));
//
//             if (in_ec) {
//               if (in_ec == boost::asio::error::operation_aborted || in_ec == boost::asio::error::broken_pipe) {
//                 goto end_complete;
//               }
//               logger_->warn(in_ec.what());
//               goto end_complete;
//             }
//
//             std::string l_line{};
//             std::istream l_is{&*l_flat_buffer};
//             std::getline(l_is, l_line);
//             while (!l_line.empty() && std::iscntrl(l_line.back(), core_set::get_set().utf8_locale))
//             l_line.pop_back(); if (!l_line.empty()) {
//               try {
//                 l_line = nlohmann::json{{"type", "logger"}, {"id", task_id_}, {"msg", l_line}}.dump();
//               } catch (const nlohmann::json::exception& e) {
//                 logger_->warn("json parse error: {}", e.what());
//                 l_line = nlohmann::json{{"type", "logger"}, {"id", task_id_}, {"msg", "日志解码错误"}}.dump();
//               }
//               l_ec = co_await websocket_client_->async_write_websocket(std::move(l_line));
//             }
//           }
//         end_complete:
//           self.complete(in_ec);
//         }
//       },
//       in_handle
//   );
// }

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
      if (l_line.back() != '\n') l_line += '\n';
      try {
        l_line = nlohmann::json{
            {"type", doodle_config::server_websocket_event::logger}, {"id", task_id_}, {"msg", l_line}
        }.dump();
      } catch (const nlohmann::json::exception& e) {
        logger_->warn("json parse error: {}", e.what());
        l_line =
            nlohmann::json{
                {"type", doodle_config::server_websocket_event::logger}, {"id", task_id_}, {"msg", "日志解码错误"}
            }
                .dump();
      }
      l_ec = co_await websocket_client_->async_write_websocket(std::move(l_line));
    }
  }
}

boost::asio::awaitable<void> http_work::async_set_status(computer_status in_status) {
  if (auto l_ec = co_await websocket_client_->async_write_websocket(
          nlohmann::json{
              {"type", doodle_config::server_websocket_event::set_state},
              {"state", in_status},
              {"host_name", host_name_},
              {"user_id", uuid_id_}
          }.dump()
      );
      l_ec) {
    logger_->error("写入错误 {}", l_ec);
  }
  co_return;
}
boost::asio::awaitable<void> http_work::async_set_task_status(server_task_info_status in_status) {
  if (auto l_ec = co_await websocket_client_->async_write_websocket(
          nlohmann::json{
              {"type", doodle_config::server_websocket_event::set_task_state}, {"id", task_id_}, {"state", in_status}
          }.dump()
      );
      l_ec) {
    logger_->error("写入错误 {}", l_ec);
  }
  co_return;
}

}  // namespace doodle::http