//
// Created by TD on 2024/2/27.
//

#include "task_info.h"

#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/task_server.h>

#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::http {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> post_task(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto l_json = std::get<nlohmann::json>(in_handle->body_);
  if (!l_json.contains("command") || !l_json["command"].is_array() || !l_json.contains("exe") ||
      !l_json["exe"].is_string()) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "请求缺失必要参数");
  }
  server_task_info l_task_handle{
      core_set::get_set().get_uuid(), l_json["exe"].get<std::string>(),
      l_json["command"].get<std::vector<std::string>>()
  };
  if (l_json.contains("source_computer") && l_json["source_computer"].is_string()) {
    l_task_handle.source_computer_ = l_json["source_computer"];
  }
  if (l_json.contains("submitter") && l_json["submitter"].is_string()) {
    l_task_handle.submitter_ = l_json["submitter"];
  }
  // 任务名称
  if (l_json.contains("name") && l_json["name"].is_string()) {
    l_task_handle.name_ = l_json["name"];
  } else {
    l_task_handle.name_ = fmt::format("task_{}", l_task_handle.id_);
  }

  l_task_handle.submit_time_ = chrono::sys_time_pos::clock::now();
  l_task_handle.log_path_    = fmt::format("task_log/{}", l_task_handle.id_);

  auto l_this_exe            = co_await boost::asio::this_coro::executor;
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  auto l_e = entt::handle{*g_reg(), g_reg()->create()};
  l_e.emplace<server_task_info>(l_task_handle);
  // g_reg()->sort<server_task_info>([](const entt::entity lhs, const entt::entity rhs) { return lhs < rhs; });
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));

  nlohmann::json l_task_handle_json{};
  l_task_handle_json = l_task_handle;
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_task_handle_json.dump();
  l_response.prepare_payload();

  co_await g_ctx().get<task_server>().add_task(l_e);
  co_return std::move(l_response);
}

boost::asio::awaitable<boost::beast::http::message_generator> get_task(session_data_ptr in_handle) {
  boost::uuids::uuid l_uuid{};
  try {
    auto l_id = in_handle->capture_->get("id");
    l_uuid    = boost::lexical_cast<boost::uuids::uuid>(l_id);
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id");
  }
  auto l_this_exe = co_await boost::asio::this_coro::executor;
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));

  server_task_info l_task_handle{};
  bool is_db = false;
  {
    auto l_view = std::as_const(*g_reg()).view<const server_task_info>();
    for (auto&& [e, l_ptr] : l_view.each()) {
      if (l_ptr.id_ == l_uuid) {
        l_task_handle = l_ptr;
        is_db         = true;
        break;
      }
    }
  }

  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));
  if (!is_db) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "任务不存在");
  }

  nlohmann::json l_json{};
  l_json = l_task_handle;
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_json.dump();
  l_response.prepare_payload();
  co_return std::move(l_response);
}

boost::asio::awaitable<boost::beast::http::message_generator> list_task(session_data_ptr in_handle) {
  auto l_url_query        = in_handle->url_.query();
  std::size_t l_page      = 0;
  std::size_t l_page_size = 100;
  try {
    if (auto l_it = l_url_query.find("page"); l_it != l_url_query.npos) {
      l_page = std::stoull(l_url_query.substr(l_it + 5, l_url_query.find('&', l_it) - l_it - 5));
    }
    if (auto l_it = l_url_query.find("page_size"); l_it != l_url_query.npos) {
      l_page_size = std::stoull(l_url_query.substr(l_it + 10, l_url_query.find('&', l_it) - l_it - 10));
    }
  } catch (const std::exception& e) {
    in_handle->logger_->error("错误的分页参数:{}", e.what());
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的分页参数");
  }
  l_page = l_page > 0 ? l_page - 1 : 0;

  auto l_this_exe = co_await boost::asio::this_coro::executor;
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  std::vector<server_task_info> l_tasks{};
  std::size_t l_view_size = 0;
  {
    auto l_view     = std::as_const(*g_reg()).view<const server_task_info>();
    l_view_size     = l_view.size();
    auto l_view_end = l_view.begin() + std::clamp((l_page + 1) * l_page_size, 0ull, l_view_size);
    for (auto it = l_view.begin() + std::clamp(l_page * l_page_size, 0ull, l_view_size); it != l_view_end; ++it) {
      auto e = *it;
      l_tasks.emplace_back(l_view.get<server_task_info>(e));
    }
  }
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));

  nlohmann::json l_tasks_json{};
  l_tasks_json["tasks"] = l_tasks;
  l_tasks_json["size"]  = l_view_size;

  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_tasks_json.dump();
  l_response.prepare_payload();
  co_return std::move(l_response);
}

boost::asio::awaitable<boost::beast::http::message_generator> get_task_logger(session_data_ptr in_handle) {
  boost::uuids::uuid l_uuid{};
  try {
    auto l_id = in_handle->capture_->get("id");
    l_uuid    = boost::lexical_cast<boost::uuids::uuid>(l_id);
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id");
  }

  server_task_info l_task_handle{l_uuid};

  level::level_enum l_level{level::err};
  auto l_query = in_handle->url_.query();
  if (auto l_it = l_query.find("level"); l_it != l_query.npos) {
    l_level = magic_enum::enum_cast<level::level_enum>(l_query.substr(l_it + 6, l_query.find('&', l_it) - l_it - 6))
                  .value_or(level::err);
  }

  auto l_this_exe = co_await boost::asio::this_coro::executor;
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  auto l_view = std::as_const(*g_reg()).view<const server_task_info>();
  for (auto&& [e, l_ptr] : l_view.each()) {
    if (l_ptr.id_ == l_task_handle.id_) {
      l_task_handle = l_ptr;
      break;
    }
  }
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));

  auto l_log_path = l_task_handle.get_log_path(l_level);
  if (!FSys::exists(l_log_path)) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "日志文件不存在");
  }
  boost::beast::http::response<boost::beast::http::file_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.set(boost::beast::http::field::content_type, "text/plain");
  boost::system::error_code l_code{};
  l_response.body().open(l_log_path.string().c_str(), boost::beast::file_mode::scan, l_code);
  if (l_code) {
    co_return in_handle->make_error_code_msg(l_code.value(), l_code.what());
  }
  l_response.prepare_payload();
  co_return std::move(l_response);
}

boost::asio::awaitable<boost::beast::http::message_generator> delete_task(session_data_ptr in_handle) {
  auto l_id = in_handle->capture_->get("id");
  boost::uuids::uuid l_uuid{};
  try {
    auto l_id = in_handle->capture_->get("id");
    l_uuid    = boost::lexical_cast<boost::uuids::uuid>(l_id);
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id");
  }

  auto l_this_exe = co_await boost::asio::this_coro::executor;
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  {
    for (auto&& [e, l_ptr] : g_reg()->view<server_task_info>().each()) {
      if (l_ptr.id_ == l_uuid) {
        g_reg()->destroy(e);
        break;
      }
    }
    // g_reg()->sort<server_task_info>([](const entt::entity lhs, const entt::entity rhs) { return lhs < rhs; });
  }
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));

  boost::beast::http::response<boost::beast::http::empty_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.prepare_payload();
  co_await g_ctx().get<task_server>().erase_task(l_uuid);
  co_return std::move(l_response);
}
}  // namespace

void task_info_reg(doodle::http::http_route& in_route) {
  in_route.reg(std::make_shared<http_function>(boost::beast::http::verb::get, "v1/task", list_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "v1/task/{id}", get_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "v1/task/{id}/log", get_task_logger))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::post, "v1/task", post_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::delete_, "v1/task/{id}", delete_task
      ));
}
} // namespace doodle::http