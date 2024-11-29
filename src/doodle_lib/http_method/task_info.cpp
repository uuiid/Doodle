//
// Created by TD on 2024/2/27.
//

#include "task_info.h"

#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

#include "computer_reg_data.h"
#include "exe_warp/maya_exe.h"
#include <spdlog/sinks/basic_file_sink.h>

namespace doodle::http {
namespace {
boost::asio::awaitable<void> task_emit(const std::shared_ptr<server_task_info>& in_ptr) {
  auto l_computer_list = computer_reg_data_manager::get().list();
  if (l_computer_list.empty()) co_return;
  for (auto&& l_com : l_computer_list) {
    if (l_com->computer_data_ptr_->uuid_id_ == in_ptr->run_computer_id_)
      if (auto l_c = l_com->client.lock(); l_c) {
        co_await l_c->async_write_websocket(
            nlohmann::json{{"type", doodle_config::work_websocket_event::post_task}, {"id", in_ptr->uuid_id_}}.dump()
        );
        co_return;
      }
  }
}

boost::asio::awaitable<boost::beast::http::message_generator> post_task(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto l_ptr = std::make_shared<server_task_info>();
  try {
    auto l_json = std::get<nlohmann::json>(in_handle->body_);
    l_json.get_to(*l_ptr);
    l_ptr->uuid_id_     = core_set::get_set().get_uuid();
    l_ptr->submit_time_ = chrono::sys_time_pos::clock::now();

    if (l_ptr->exe_.empty())
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "运行程序任务为空");

    if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<computer>(l_ptr->run_computer_id_); l_list == 0)
      co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到计算机");

  } catch (...) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::current_exception_diagnostic_information()
    );
  }

  if (auto l_e = co_await g_ctx().get<sqlite_database>().install(l_ptr); !l_e)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_e.error());

  boost::asio::co_spawn(g_io_context(), task_emit(l_ptr), boost::asio::consign(boost::asio::detached, l_ptr));
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> get_task(session_data_ptr in_handle) {
  uuid l_uuid{};
  try {
    auto l_id = in_handle->capture_->get("id");
    l_uuid    = boost::lexical_cast<boost::uuids::uuid>(l_id);
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id");
  }
  if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<server_task_info>(l_uuid); !l_list.empty())
    co_return in_handle->make_msg((nlohmann::json{} = l_list[0]).dump());

  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "任务不存在");
}

boost::asio::awaitable<boost::beast::http::message_generator> list_task(session_data_ptr in_handle) {
  uuid l_user_uuid{};
  try {
    for (auto&& i : in_handle->url_.params()) {
      if (i.has_value && i.key == "user_id") {
        l_user_uuid = boost::lexical_cast<boost::uuids::uuid>(i.value);
      }
    }
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id");
  }
  if (!l_user_uuid.is_nil()) {
    if (auto l_list = g_ctx().get<sqlite_database>().get_server_task_info_by_user(l_user_uuid); !l_list.empty())
      co_return in_handle->make_msg((nlohmann::json{} = l_list[0]).dump());
    co_return in_handle->make_msg("[]"s);
  }
  co_return in_handle->make_msg((nlohmann::json{} = g_ctx().get<sqlite_database>().get_all<server_task_info>()).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> get_task_logger(session_data_ptr in_handle) {
  boost::uuids::uuid l_uuid{};
  try {
    auto l_id = in_handle->capture_->get("id");
    l_uuid    = boost::lexical_cast<boost::uuids::uuid>(l_id);
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id");
  }
  auto l_path =
      core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", l_uuid);
  if (!FSys::exists(l_path))
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "日志不存在");
  auto l_ex = in_handle->make_msg(l_path, kitsu::mime_type(l_path.extension()));
  if (!l_ex) co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_ex.error());
  co_return std::move(*l_ex);
}

boost::asio::awaitable<boost::beast::http::message_generator> delete_task(session_data_ptr in_handle) {
  auto l_id   = in_handle->capture_->get("id");
  auto l_uuid = std::make_shared<uuid>();
  try {
    auto l_id = in_handle->capture_->get("id");
    *l_uuid   = boost::lexical_cast<boost::uuids::uuid>(l_id);
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id");
  }

  if (auto l_list = co_await g_ctx().get<sqlite_database>().remove<server_task_info>(l_uuid); !l_list) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_list.error());
  }
  co_return in_handle->make_msg("{}");
}

template <class Mutex>
class run_post_task_local_impl_sink : public spdlog::sinks::base_sink<Mutex> {
  std::shared_ptr<server_task_info> task_info_{};
  std::once_flag flag_;

 public:
  explicit run_post_task_local_impl_sink(std::shared_ptr<server_task_info> in_task_info) : task_info_(in_task_info) {}
  void sink_it_(const spdlog::details::log_msg& msg) override { std::call_once(flag_, set_state); }
  void flush_() override {}
  void set_state() {
    task_info_->status_ = server_task_info_status::running;
    boost::asio::co_spawn(g_io_context(), g_ctx().get<sqlite_database>().install(task_info_), boost::asio::detached);
  }
};
using run_post_task_local_impl_sink_mt = run_post_task_local_impl_sink<std::mutex>;

boost::asio::awaitable<void> run_post_task_local_impl(
    std::shared_ptr<server_task_info> in_task_info, std::shared_ptr<maya_exe_ns::arg> in_arg, logger_ptr in_logger
) {
  in_logger->sinks().emplace_back(std::make_shared<run_post_task_local_impl_sink_mt>(in_task_info));
  auto [l_e, l_r] = co_await async_run_maya(in_arg, in_logger);
  if (l_e) {
    in_task_info->status_ = server_task_info_status::failed;
  } else
    in_task_info->status_ = server_task_info_status::completed;
  auto l_t = co_await g_ctx().get<sqlite_database>().install(in_task_info);
}
boost::asio::awaitable<boost::beast::http::message_generator> post_task_local(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto l_ptr = std::make_shared<server_task_info>();
  std::shared_ptr<maya_exe_ns::arg> l_arg{};
  logger_ptr l_logger_ptr{};
  try {
    auto l_json = std::get<nlohmann::json>(in_handle->body_);
    l_json.get_to(*l_ptr);
    l_ptr->uuid_id_         = core_set::get_set().get_uuid();
    l_ptr->submit_time_     = chrono::sys_time_pos::clock::now();
    l_ptr->run_computer_id_ = boost::uuids::nil_uuid();
    if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);

    auto& l_task = l_json["task_data"];
    if (l_task.contains("replace_ref_file")) {
      auto l_arg_t = std::make_shared<maya_exe_ns::qcloth_arg>();
      l_task.get_to(*l_arg_t);
      l_arg = l_arg_t;
    } else if (l_task.contains("file_list")) {
      auto l_arg_t = std::make_shared<maya_exe_ns::replace_file_arg>();
      l_task.get_to(*l_arg_t);
      l_arg = l_arg_t;
    } else {
      auto l_arg_t = std::make_shared<maya_exe_ns::export_fbx_arg>();
      l_task.get_to(*l_arg_t);
      l_arg = l_arg_t;
    }

    if (l_ptr->exe_.empty())
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "运行程序任务为空");
    auto l_logger_path = core_set::get_set().get_cache_root() / server_task_info::logger_category /
                         fmt::format("{}.log", l_ptr->uuid_id_);
    l_logger_ptr = std::make_shared<spdlog::async_logger>(
        l_ptr->name_, std::make_shared<spdlog::sinks::basic_file_sink_mt>(l_logger_path.generic_string()),
        spdlog::thread_pool()
    );
  } catch (...) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::current_exception_diagnostic_information()
    );
  }
  if (!l_arg) co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的参数");

  if (auto l_e = co_await g_ctx().get<sqlite_database>().install(l_ptr); !l_e)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_e.error());

  boost::asio::co_spawn(
      g_io_context(), run_post_task_local_impl(l_ptr, l_arg, l_logger_ptr),
      boost::asio::consign(boost::asio::detached, l_arg)
  );
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

}  // namespace

void task_info_reg(doodle::http::http_route& in_route) {
  in_route.reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task", list_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{id}", get_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{id}/log", get_task_logger))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::post, "api/doodle/task", post_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::delete_, "api/doodle/task/{id}", delete_task));
}
void task_info_reg_local(doodle::http::http_route& in_route) {
  if (!g_ctx().contains<maya_ctx>()) g_ctx().emplace<maya_ctx>();
  in_route.reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task", list_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{id}", get_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{id}/log", get_task_logger))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::post, "api/doodle/task", post_task_local))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::delete_, "api/doodle/task/{id}", delete_task));
}
}  // namespace doodle::http