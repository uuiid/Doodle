//
// Created by td_main on 2023/8/17.
//
#include "url_route_put.h"

namespace doodle::render_farm {
namespace detail {
void render_job_type_put::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap)
    const {
  auto& l_session        = in_handle.get<working_machine_session>();
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
  auto l_parser_ptr      = std::make_shared<json_parser_type>(std::move(l_session.request_parser()));
  boost::beast::http::async_read(
      l_session.stream(), l_session.buffer(), *l_parser_ptr,
      [in_handle, l_parser_ptr](boost::system::error_code ec, std::size_t bytes_transferred) {
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

        auto l_h = entt::handle{*g_reg(), g_reg()->create()};
        l_h.emplace<process_message>();
        try {
          l_h.emplace<ue4_task>(l_h, l_parser_ptr->release().body().get<ue4_task::arg_t>());
        } catch (const nlohmann::json::exception& e) {
          DOODLE_LOG_ERROR("json parse error: {}", e.what());
          l_session.send_error(e);
          return;
        }

        boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
        l_response.body() = {{"state", "ok"}, {"id", l_h.entity()}};
        l_response.keep_alive(l_parser_ptr->keep_alive());
        l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
      }
  );
}
}  // namespace detail
}  // namespace doodle::render_farm