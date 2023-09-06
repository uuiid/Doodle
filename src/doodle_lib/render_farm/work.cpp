//
// Created by td_main on 2023/8/21.
//

#include "work.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/render_farm/detail/computer.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>
#include <doodle_lib/render_farm/udp_client.h>
#include <doodle_lib/render_farm/working_machine_session.h>

#include <boost/beast.hpp>
#include <boost/url.hpp>
namespace doodle {
namespace render_farm {
void work::on_wait(boost::system::error_code ec) {
  if (ec == boost::asio::error::operation_aborted) {
    return;
  }
  if (ec) {
    DOODLE_LOG_INFO("{}", ec);
    return;
  }
  do_register();
}
void work::do_wait() {
  DOODLE_LOG_INFO("开始等待下一次心跳");
  ptr_->timer_->expires_after(doodle::chrono::seconds{2});
  ptr_->timer_->async_wait(std::bind(&work::on_wait, this, std::placeholders::_1));
}

void work::make_ptr() {
  auto l_s          = boost::asio::make_strand(g_io_context());
  ptr_->timer_      = std::make_shared<timer>(l_s);
  ptr_->signal_set_ = std::make_shared<signal_set>(l_s, SIGINT, SIGTERM);
  ptr_->signal_set_->async_wait([&](boost::system::error_code ec, int signal) {
    if (ec) {
      DOODLE_LOG_ERROR("signal_set_ error: {}", ec);
      return;
    }
    DOODLE_LOG_INFO("signal_set_ signal: {}", signal);
    this->do_close();
  });
  ptr_->udp_client_ptr_ = std::make_shared<doodle::udp_client>(g_io_context());
}

void work::run() { find_server_address(); }
void work::do_register() {
  boost::url l_url{"/v1/render_farm/computer"};
  auto l_view = g_reg()->view<render_ue4>();
  l_url.params().set("status", magic_enum::enum_name(l_view.empty() ? computer_status::idle : computer_status::busy));
  if (ptr_->computer_id != entt::null) l_url.params().set("id", fmt::to_string(ptr_->computer_id));
  l_url.remove_origin();

  request_type l_request{boost::beast::http::verb::post, l_url.c_str(), 11};
  l_request.set(boost::beast::http::field::content_type, "application/json");
  l_request.set(boost::beast::http::field::accept, "application/json");
  nlohmann::json l_json;
  l_json["name"]   = boost::asio::ip::host_name();
  l_request.body() = l_json.dump();
  l_request.prepare_payload();
  using response_type_1 = boost::beast::http::response<boost::beast::http::string_body>;

  ptr_->core_ptr_->async_read<response_type_1>(
      boost::asio::make_strand(g_io_context()), l_request,
      [this](auto&& PH1, const response_type_1& PH2) {
        if (PH2.result() == boost::beast::http::status::ok) {
          try {
            auto l_json       = nlohmann::json::parse(PH2.body());
            ptr_->computer_id = num_to_enum<entt::entity>(l_json["id"].get<std::int32_t>());
            DOODLE_LOG_INFO("computer_id: {}", ptr_->computer_id);
          } catch (const nlohmann::json::exception& e) {
            DOODLE_LOG_ERROR("json parse error: {}", boost::diagnostic_information(e));
          }
        } else {
          DOODLE_LOG_ERROR("未注册成功 {}", PH2.result_int());
        }
        do_wait();
      }
  );
}
void work::send_server_state() {
  request_type l_request{
      boost::beast::http::verb::post, fmt::format("/v1/render_farm/render_job/{}", ptr_->ue_data_ptr_->server_id), 11};
  l_request.set(boost::beast::http::field::content_type, "application/json");
  l_request.set(boost::beast::http::field::accept, "application/json");
  nlohmann::json l_json;
  l_json["state"]  = ptr_->ue_data_ptr_->run_handle.get<process_message>().get_state();

  l_request.body() = l_json.dump();
  l_request.prepare_payload();
  using response_type_1 = boost::beast::http::response<boost::beast::http::string_body>;

  ptr_->core_ptr_->async_read<response_type_1>(
      boost::asio::make_strand(g_io_context()), l_request,
      [this](auto&& PH1, const response_type_1& PH2) {
        DOODLE_LOG_INFO("{}", PH2.body());
        ptr_->ue_data_ptr_.reset();
        do_wait();
      }
  );
}

void work::do_close() {
  if (ptr_->core_ptr_) ptr_->core_ptr_->cancel();
}
bool work::find_server_address(std::uint16_t in_port) {
  ptr_->udp_client_ptr_->async_find_server(
      in_port,
      [this](auto&& PH1, boost::asio::ip::udp::endpoint& in_remove_endpoint) {
        if (PH1) {
          DOODLE_LOG_ERROR("{}", PH1);
          find_server_address();
          return;
        }
        boost::ignore_unused(PH1);
        auto l_remote_address         = in_remove_endpoint.address().to_string();
        core_set::get_set().server_ip = l_remote_address;
        DOODLE_LOG_INFO("收到服务器响应 {}", l_remote_address);
        ptr_->core_ptr_ = std::make_shared<client_core>(std::move(l_remote_address));
        do_register();
      }
  );
  return true;
}

void work::run_job(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap) {
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
          DOODLE_LOG_ERROR("on_read error: {}", ec.message());
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
        } catch (const nlohmann::json::exception& e) {
          DOODLE_LOG_ERROR("json parse error: {}", e.what());
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

}  // namespace render_farm
}  // namespace doodle