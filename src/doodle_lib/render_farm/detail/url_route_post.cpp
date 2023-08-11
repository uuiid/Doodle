//
// Created by td_main on 2023/8/9.
//

#include "url_route_post.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>

#include <doodle_lib/render_farm/detail/computer.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>
namespace doodle::render_farm {
namespace detail {

void http_method<boost::beast::http::verb::post>::run(std::shared_ptr<working_machine_session> in_session) {
  auto l_m = parser(chick_url(in_session->url_.segments()));

  if (map_action.count(l_m) == 0) {
    boost::beast::http::response<boost::beast::http::empty_body> l_response{boost::beast::http::status::not_found, 11};
    in_session->send_response(boost::beast::http::message_generator{std::move(l_response)});
  } else {
    map_action.at(l_m)(in_session, in_session->url_.params());
  }
}

void http_method<boost::beast::http::verb::post>::render_job(std::shared_ptr<working_machine_session> in_session) {
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
  auto l_parser_ptr      = std::make_shared<json_parser_type>(std::move(in_session->request_parser_));
  boost::beast::http::async_read(
      in_session->stream_, in_session->buffer_, *l_parser_ptr,
      [self = in_session, l_parser_ptr](boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec == boost::beast::http::error::end_of_stream) {
          return self->do_close();
        }
        if (ec) {
          DOODLE_LOG_ERROR("on_read error: {}", ec.message());
          self->send_error_code(ec);
          return;
        }

        auto l_h = entt::handle{*g_reg(), g_reg()->create()};
        l_h.emplace<process_message>();
        try {
          l_h.emplace<render_ue4_ptr>(std::make_shared<render_ue4_ptr ::element_type>(
              l_h, l_parser_ptr->release().body().get<render_ue4_ptr::element_type::arg>()
          ));
        } catch (const nlohmann::json::exception& e) {
          DOODLE_LOG_ERROR("json parse error: {}", e.what());
          self->send_error(e);
          return;
        }

        boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
        l_response.body() = {{"state", "ok"}, {"id", l_h.entity()}};
        l_response.keep_alive(l_parser_ptr->get().keep_alive());
        self->send_response(boost::beast::http::message_generator{std::move(l_response)});
      }
  );
}
void http_method<boost::beast::http::verb::post>::computer_reg(std::shared_ptr<working_machine_session> in_session) {
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
  auto l_parser_ptr      = std::make_shared<json_parser_type>(std::move(in_session->request_parser_));
  boost::beast::http::async_read(
      in_session->stream_, in_session->buffer_, *l_parser_ptr,
      [self = in_session, l_parser_ptr](boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec == boost::beast::http::error::end_of_stream) {
          return self->do_close();
        }
        if (ec) {
          DOODLE_LOG_ERROR("on_read error: {}", ec.message());
          self->send_error_code(ec);
          return;
        }

        auto l_json = l_parser_ptr->release().body();
        entt::handle l_handle{};
        if (l_json.contains("id")) {
          l_handle = entt::handle{*g_reg(), l_json["id"].get<entt::entity>()};
        }
        if (!l_handle) {
          try {
            auto l_c = l_json.get<computer>();
            g_reg()->view<computer>().each([&](const entt::entity& e, computer& in_computer) {
              if (in_computer.name() == l_c.name()) {
                l_handle = entt::handle{*g_reg(), e};
              }
            });
          } catch (const nlohmann::json::exception& e) {
            DOODLE_LOG_ERROR("json parse error: {}", e.what());
            self->send_error(e);
            return;
          }
        }
        if (!l_handle) {
          l_handle = entt::handle{*g_reg(), g_reg()->create()};
          l_handle.emplace<computer>(l_json.get<computer>());
        }
        if (l_json.contains("status")) {
          l_handle.get<computer>().delay(l_json["status"].get<std::string>());
        } else {
          l_handle.get<computer>().delay();
        }
        boost::beast::http::response<basic_json_body> l_response{boost::beast::http::status::ok, 11};
        l_response.keep_alive(l_parser_ptr->keep_alive());
        l_response.body() = {{"state", "ok"}, {"id", l_handle.entity()}};
        self->send_response(boost::beast::http::message_generator{std::move(l_response)});
      }
  );
}

}  // namespace detail
}  // namespace doodle::render_farm