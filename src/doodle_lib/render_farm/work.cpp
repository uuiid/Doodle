//
// Created by td_main on 2023/8/21.
//

#include "work.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/render_farm/detail/computer.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>
#include <doodle_lib/render_farm/http_session.h>
#include <doodle_lib/render_farm/udp_client.h>

#include <boost/beast.hpp>
#include <boost/url.hpp>
namespace doodle {
namespace render_farm {

void work::do_wait() {
  log_info(ptr_->logger_, "开始等待下一次心跳");
  ptr_->timer_->expires_after(doodle::chrono::seconds{2});
  ptr_->timer_->async_wait([this](boost::system::error_code ec) {
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }
    if (ec) {
      log_info(ptr_->logger_, fmt::format("on_wait error: {}", ec));
      return;
    }
    if (!ptr_->websocket_handle) {
      do_connect();
      return;
    }
    if (ptr_->websocket_handle.get<websocket_data>().is_handshake_)
      do_register();
    else
      do_connect();
  });
}

void work::make_ptr() {
  auto l_s                = boost::asio::make_strand(g_io_context());
  ptr_->timer_            = std::make_shared<timer>(l_s);
  ptr_->send_log_timer_   = std::make_shared<timer>(l_s);
  ptr_->send_error_timer_ = std::make_shared<timer>(l_s);
  ptr_->signal_set_       = std::make_shared<signal_set>(l_s, SIGINT, SIGTERM);
  ptr_->logger_           = g_logger_ctrl().make_log(fmt::format("{} {}", typeid(*this).name(), fmt::ptr(this)));
  ptr_->signal_set_->async_wait([&, l_logger = ptr_->logger_](boost::system::error_code ec, int signal) {
    if (ec) {
      log_info(l_logger, fmt::format("signal_set_ error: {}", ec));
      return;
    }
    log_info(l_logger, fmt::format("signal_set_ signal: {}", signal));
    this->do_close();
  });
}

void work::run(const std::string& in_server_ip, std::uint16_t in_port) {
  boost::ignore_unused(in_port);
  ptr_->core_ptr_       = std::make_shared<client_core>(in_server_ip);
  ptr_->server_address_ = in_server_ip;
  do_connect();
}

void work::do_connect() {
  ptr_->websocket_handle = entt::handle{*g_reg(), g_reg()->create()};
  auto l_websocket_ptr_  = std::make_shared<websocket>(ptr_->websocket_handle);
  l_websocket_ptr_->async_connect(
      ptr_->server_address_, "/v1/render_farm/computer", doodle_config::http_port,
      [this](boost::system::error_code ec) {
        if (ec) {
          log_info(ptr_->logger_, fmt::format("连接失败 {}", ec));
          do_wait();
          return;
        }
        log_info(ptr_->logger_, "连接成功");
        do_register();
      }
  );
}

void work::do_register() {
  if (!ptr_->websocket_handle) {
    do_wait();
    return;
  }
  nlohmann::json l_json{};
  l_json["method"] = "run.reg.computer";
  l_json["params"]["status"] =
      magic_enum::enum_name(ptr_->ue_data_ptr_ ? computer_status::busy : computer_status::idle);
  auto l_logger = ptr_->websocket_handle.get<socket_logger>().logger_;
  ptr_->websocket_handle.get<websocket_data>().stream_.ping({});
  ptr_->websocket_handle.get<websocket_data>().websocket_ptr_->async_call(
      l_json,
      [l_data = ptr_, l_logger, this](boost::system::error_code ec, const nlohmann::json& in_json) {
        if (ec) {
          log_info(l_logger, fmt::format("注册失败 {}", ec));
          l_data->websocket_handle.get<websocket_data>().websocket_ptr_->close();
          l_data->computer_id = entt::null;
          do_wait();
          return;
        }
        if (in_json.contains("error")) {
          log_info(l_logger, fmt::format("注册失败 {}", in_json["error"]["message"].get<std::string>()));
          l_data->computer_id = entt::null;
        } else {
          if (in_json["result"].contains("id")) {
            l_data->computer_id = num_to_enum<entt::entity>(in_json["result"]["id"].get<std::int32_t>());
            log_info(l_logger, fmt::format("computer_id: {}", l_data->computer_id));
          } else {
            log_error(l_logger, fmt::format("注册失败 {}", in_json["result"].dump()));
            l_data->computer_id = entt::null;
          }
        }
        do_wait();
      }
  );
}

void work::send_log(std::string in_log) {
  ptr_->log_cache_ += std::move(in_log);
  ptr_->send_log_timer_->expires_after(doodle::chrono::seconds{1});
  ptr_->send_log_timer_->async_wait([this](boost::system::error_code ec) {
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }
    if (ec) {
      log_info(ptr_->logger_, fmt::format("on_wait error: {}", ec));
      return;
    }
    send_log_impl();
  });
}
void work::send_err(std::string in_err) {
  ptr_->err_cache_ += std::move(in_err);
  ptr_->send_error_timer_->expires_after(doodle::chrono::seconds{1});
  ptr_->send_error_timer_->async_wait([this](boost::system::error_code ec) {
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }
    if (ec) {
      log_info(ptr_->logger_, fmt::format("on_wait error: {}", ec));
      return;
    }
    send_error_impl();
  });
}
void work::do_close() {
  if (ptr_->core_ptr_) ptr_->core_ptr_->cancel();
  if (ptr_->websocket_handle) ptr_->websocket_handle.get<websocket_data>().websocket_ptr_->close();
  ptr_->timer_->cancel();
}

void work::send_server_state() {
  if (!ptr_->core_ptr_) {
    log_error(ptr_->logger_, "core_ptr_ is not valid");
    return;
  }

  request_type l_request{
      boost::beast::http::verb::put, fmt::format("/v1/render_farm/render_job/{}", ptr_->ue_data_ptr_->server_id), 11};
  l_request.set(boost::beast::http::field::content_type, "application/json");
  l_request.set(boost::beast::http::field::accept, "application/json");
  nlohmann::json l_json;
  auto l_state    = ptr_->ue_data_ptr_->run_handle.get<process_message>().get_state();
  l_json["state"] = l_state;
  log_info(ptr_->core_ptr_->logger(), fmt::format("send_server_state {}", l_state));

  l_request.body() = l_json.dump();
  l_request.prepare_payload();
  using response_type_1 = boost::beast::http::response<boost::beast::http::string_body>;

  ptr_->core_ptr_->async_read<response_type_1>(
      boost::asio::make_strand(g_io_context()), l_request,
      [this, l_state](auto&& PH1, const response_type_1& PH2) {
        if (PH1) {
          log_error(ptr_->core_ptr_->logger(), fmt::format("{}", PH1));
        }
        if (PH2.result() != boost::beast::http::status::ok) {
          log_warn(ptr_->core_ptr_->logger(), fmt::format("服务器回复 {} 错误", PH2.result_int()));
        }
        log_info(ptr_->core_ptr_->logger(), fmt::format("{}", PH2.body()));
        if (l_state == process_message::state::success || l_state == process_message::state::fail)
          ptr_->ue_data_ptr_.reset();
        do_wait();
      }
  );
}

void work::send_log_impl() {
  request_type l_request{
      boost::beast::http::verb::post, fmt::format("/v1/render_farm/log/{}", ptr_->ue_data_ptr_->server_id), 11};
  l_request.set(boost::beast::http::field::content_type, "plain/text");
  l_request.set(boost::beast::http::field::accept, "application/json");

  l_request.body() = std::move(ptr_->log_cache_);
  ptr_->log_cache_ = {};
  l_request.prepare_payload();
  using response_type_1 = boost::beast::http::response<boost::beast::http::string_body>;
  ptr_->core_ptr_->async_read<response_type_1>(
      boost::asio::make_strand(g_io_context()), l_request,
      [this](auto&& PH1, const response_type_1& PH2) {
        log_info(ptr_->core_ptr_->logger(), fmt::format("{}", PH2.body()));
      }
  );
}
void work::send_error_impl() {
  request_type l_request{
      boost::beast::http::verb::post, fmt::format("/v1/render_farm/err/{}", ptr_->ue_data_ptr_->server_id), 11};
  l_request.set(boost::beast::http::field::content_type, "plain/text");
  l_request.set(boost::beast::http::field::accept, "application/json");

  l_request.body() = std::move(ptr_->err_cache_);
  ptr_->err_cache_ = {};
  l_request.prepare_payload();
  using response_type_1 = boost::beast::http::response<boost::beast::http::string_body>;
  ptr_->core_ptr_->async_read<response_type_1>(
      boost::asio::make_strand(g_io_context()), l_request,
      [this](auto&& PH1, const response_type_1& PH2) {
        log_info(ptr_->core_ptr_->logger(), fmt::format("{}", PH2.body()));
      }
  );
}
void work::stop() {
  ptr_->core_ptr_->cancel();
  if (ptr_->websocket_handle) ptr_->websocket_handle.get<websocket_data>().websocket_ptr_->close();
  ptr_->timer_->cancel();
}
boost::system::error_code work::run_job(const entt::handle& in_handle, const nlohmann::json& in_json) {
  auto l_ue = std::make_shared<ue_data>();
  boost::system::error_code ec{};
  try {
    l_ue->server_id = in_json["id"].get<entt::entity>();
    auto l_h        = entt::handle{*g_reg(), g_reg()->create()};
    l_h.emplace<process_message>();
    l_h.emplace<render_ue4>(l_h, in_json["arg"].get<render_ue4::arg_t>()).run();
    l_ue->run_handle = l_h;
  } catch (const nlohmann::json::exception& e) {
    log_info(ptr_->core_ptr_->logger(), fmt::format("json parse error: {}", boost::diagnostic_information(e)));
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::bad_json_string);
    return ec;
  }
  ptr_->ue_data_ptr_ = l_ue;
  return ec;
}

}  // namespace render_farm
}  // namespace doodle