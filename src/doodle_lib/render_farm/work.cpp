//
// Created by td_main on 2023/8/21.
//

#include "work.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/render_farm/detail/computer.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>

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
  run();
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
}

void work::run() {
  boost::url l_url{"/v1/render_farm/computer"};
  auto l_view = g_reg()->view<render_ue4>();
  l_url.params().set("status", magic_enum::enum_name(l_view.empty() ? computer_status::idle : computer_status::busy));
  if (ptr_->computer_id != entt::null) l_url.params().set("id", fmt::to_string(ptr_->computer_id));

  request_type l_request{boost::beast::http::verb::post, "/v1/render_farm/computer", 11};
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
          auto l_json       = nlohmann::json::parse(PH2.body());
          ptr_->computer_id = num_to_enum<entt::entity>()(l_json["id"].get<std::int32_t>());
        }
        DOODLE_LOG_INFO("{}", PH2.body());
        do_wait();
      }
  );
}

void work::send_server_state(const entt::handle& in_handle) {
  entt::entity l_id = in_handle.get<detail::ue_server_id>();
  request_type l_request{boost::beast::http::verb::post, fmt::format("/v1/render_farm/render_job/{}", l_id), 11};
  l_request.set(boost::beast::http::field::content_type, "application/json");
  l_request.set(boost::beast::http::field::accept, "application/json");
  nlohmann::json l_json;
  l_json["state"]  = in_handle.get<process_message>().get_state();

  l_request.body() = l_json.dump();
  l_request.prepare_payload();
  using response_type_1 = boost::beast::http::response<boost::beast::http::string_body>;

  ptr_->core_ptr_->async_read<response_type_1>(
      boost::asio::make_strand(g_io_context()), l_request,
      [this, in_handle](auto&& PH1, const response_type_1& PH2) {
        DOODLE_LOG_INFO("{}", PH2.body());
        auto l_h = in_handle;
        l_h.destroy();
        do_wait();
      }
  );
}

void work::do_close() { ptr_->core_ptr_->cancel(); }

}  // namespace render_farm
}  // namespace doodle