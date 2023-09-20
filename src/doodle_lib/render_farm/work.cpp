//
// Created by td_main on 2023/8/21.
//

#include "work.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/render_farm/detail/computer.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>
#include <doodle_lib/render_farm/udp_client.h>
#include <doodle_lib/render_farm/working_machine_session.h>

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
    do_register();
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
  ptr_->websocket_ptr_ = std::make_shared<websocket>(entt::handle{*g_reg(), g_reg()->create()});
}

void work::run(const std::string& in_server_ip, std::uint16_t in_port) {
  boost::ignore_unused(in_port);
  ptr_->core_ptr_ = std::make_shared<client_core>(in_server_ip);
  ptr_->websocket_ptr_->run(in_server_ip, "/v1/render_farm/computer", doodle_config::http_port);
  do_register();
}
void work::do_register() {
  nlohmann::json l_json{};
  l_json["method"]           = "run.reg.computer";
  l_json["params"]["status"] = ptr_->ue_data_ptr_ ? computer_status::busy : computer_status::idle;

  ptr_->websocket_ptr_->async_call(l_json, [this](const nlohmann::json& in_json) {
    if (in_json.contains("error")) {
      log_info(ptr_->logger_, fmt::format("注册失败 {}", in_json["error"]["message"].get<std::string>()));
      ptr_->computer_id = entt::null;
    } else {
      ptr_->computer_id = num_to_enum<entt::entity>(in_json["result"]["id"].get<std::int32_t>());
      log_info(ptr_->logger_, fmt::format("computer_id: {}", ptr_->computer_id));
    }
    do_wait();
  });
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

void work::run_job(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap) {
  boost::ignore_unused(in_cap);
  auto& l_session        = in_handle.get<working_machine_session>();
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
  auto l_parser_ptr      = std::make_shared<json_parser_type>(std::move(l_session.request_parser()));

  boost::beast::http::async_read(
      l_session.stream(), l_session.buffer(), *l_parser_ptr,
      [in_handle, l_parser_ptr, this](boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        auto& l_session = in_handle.get<working_machine_session>();
        if (ec == boost::beast::http::error::end_of_stream) {
          return l_session.do_close();
        }
        if (ec) {
          log_info(ptr_->core_ptr_->logger(), fmt::format("on_read error: {}", ec.message()));
          l_session.send_error_code(ec);
          return;
        }

        if (ptr_->ue_data_ptr_) {
          BOOST_BEAST_ASSIGN_EC(ec, error_enum::not_allow_multi_work);
          l_session.send_error_code(ec, boost::beast::http::status::service_unavailable);
          return;
        }

        auto l_json = l_parser_ptr->release().body();

        auto l_ue   = std::make_shared<ue_data>();
        try {
          l_ue->server_id = l_json["id"].get<entt::entity>();
          auto l_h        = entt::handle{*g_reg(), g_reg()->create()};
          l_h.emplace<process_message>();
          l_h.emplace<render_ue4>(l_h, l_json["arg"].get<render_ue4::arg_t>()).run();
          l_ue->run_handle = l_h;
        } catch (const nlohmann::json::exception& e) {
          log_info(ptr_->core_ptr_->logger(), fmt::format("json parse error: {}", boost::diagnostic_information(e)));
          l_session.send_error(e);
          return;
        }
        ptr_->ue_data_ptr_ = l_ue;

        boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
        l_response.keep_alive(l_parser_ptr->keep_alive());
        l_response.body() = {{"state", "ok"}};
        l_response.prepare_payload();
        l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
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