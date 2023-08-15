//
// Created by td_main on 2023/8/9.
//

#include "url_route_post.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>

#include <doodle_lib/render_farm/detail/aync_read_body.h>
#include <doodle_lib/render_farm/detail/computer.h>
#include <doodle_lib/render_farm/detail/forward_to_server.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>
#include <doodle_lib/render_farm/detail/ue4_task.h>
namespace doodle::render_farm {
namespace detail {

void http_method<boost::beast::http::verb::post>::run(const entt::handle& in_handle) {
  auto& l_session = in_handle.get<working_machine_session>();
  auto l_m        = parser(chick_url(l_session.url().segments()));

  if (map_action.count(l_m) == 0) {
    boost::beast::http::response<boost::beast::http::empty_body> l_response{boost::beast::http::status::not_found, 11};
    l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
  } else {
    map_action.at(l_m)(in_handle);
  }
}

void http_method<boost::beast::http::verb::post>::render_job(const entt::handle& in_handle) {
  auto& l_session        = in_handle.get<working_machine_session>();
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
  auto l_parser_ptr      = std::make_shared<json_parser_type>(std::move(l_session.request_parser()));
  boost::beast::http::async_read(
      l_session.stream_, l_session.buffer_, *l_parser_ptr,
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
          l_h.emplace<ue4_task_ptr>(std::make_shared<ue4_task_ptr::element_type>(
              l_h, l_parser_ptr->release().body().get<ue4_task_ptr::element_type::arg>()
          ));
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
void http_method<boost::beast::http::verb::post>::computer_reg(const entt::handle& in_handle) {
  auto& l_session        = in_handle.get<working_machine_session>();
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
  auto l_parser_ptr      = std::make_shared<json_parser_type>(std::move(l_session.request_parser()));
  boost::beast::http::async_read(
      l_session.stream_, l_session.buffer_, *l_parser_ptr,
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

        auto l_json      = l_parser_ptr->release().body();
        auto l_source_ip = l_session.stream().socket().remote_endpoint().address().to_string();
        DOODLE_LOG_INFO("computer_reg: {}", l_source_ip);
        entt::handle l_handle{};
        auto l_p = l_session.url().params();

        if (auto l_it = l_p.find("id"); l_it != l_p.end()) {
          l_handle = entt::handle{*g_reg(), num_to_enum<entt::entity>(std::stoi((*l_it).value))};
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
            l_session.send_error(e);
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
        l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
      }
  );
}
void http_method<boost::beast::http::verb::post>::run_job(const entt::handle& in_handle) {
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
  auto& l_session        = in_handle.get<working_machine_session>();
  auto l_parser_ptr      = std::make_shared<json_parser_type>(std::move(l_session.request_parser()));
  boost::beast::http::async_read(
      l_session.stream_, l_session.buffer_, *l_parser_ptr,
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

        auto l_json = l_parser_ptr->release().body();

        auto l_h    = entt::handle{*g_reg(), g_reg()->create()};
        l_h.emplace<process_message>();
        try {
          l_h.emplace<ue_server_id>(l_json["id"].get<entt::entity>());
          auto l_task_ptr = std::make_shared<render_ue4_ptr::element_type>(
              l_h, l_json["arg"].get<render_ue4_ptr::element_type::arg>()
          );
          l_h.emplace<render_ue4_ptr>(l_task_ptr);
        } catch (const nlohmann::json::exception& e) {
          DOODLE_LOG_ERROR("json parse error: {}", e.what());
          l_session.send_error(e);
          return;
        }

        boost::beast::http::response<basic_json_body> l_response{boost::beast::http::status::ok, 11};
        l_response.keep_alive(l_parser_ptr->keep_alive());
        l_response.body() = {{"state", "ok"}};
        l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
      }
  );
}
void http_method<boost::beast::http::verb::post>::client_submit_job(const entt::handle& in_handle) {
  using parser_type = boost::beast::http::request_parser<boost::beast::http::string_body>;
  auto& l_session   = in_handle.get<working_machine_session>();
  auto l_parser_ptr = std::make_shared<parser_type>(std::move(l_session.request_parser()));
  boost::beast::http::async_read(
      l_session.stream_, l_session.buffer_, *l_parser_ptr, in_handle.emplace<forward_to_server>(in_handle, l_parser_ptr)
  );
}

}  // namespace detail
}  // namespace doodle::render_farm