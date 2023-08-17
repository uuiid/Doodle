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

void render_job_type_post::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap)
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
void client_submit_job_type_post::operator()(
    const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap
) const {
  using parser_type = boost::beast::http::request_parser<boost::beast::http::string_body>;
  auto& l_session   = in_handle.get<working_machine_session>();
  auto l_parser_ptr = std::make_shared<parser_type>(std::move(l_session.request_parser()));
  boost::beast::http::async_read(
      l_session.stream(), l_session.buffer(), *l_parser_ptr,
      in_handle.emplace<forward_to_server>(in_handle, l_parser_ptr)
  );
}

void computer_reg_type_post::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap)
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
void run_job_post::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap) const {
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
  auto& l_session        = in_handle.get<working_machine_session>();
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

        auto l_json = l_parser_ptr->release().body();

        auto l_h    = entt::handle{*g_reg(), g_reg()->create()};
        l_h.emplace<process_message>();
        try {
          l_h.emplace<ue_server_id>(l_json["id"].get<entt::entity>());

          l_h.emplace<render_ue4>(l_h, l_json["arg"].get<render_ue4::arg_t>());
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
}  // namespace detail
}  // namespace doodle::render_farm