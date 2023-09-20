//
// Created by td_main on 2023/8/9.
//

#include "url_route_post.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_url.h>

#include <doodle_lib/render_farm/detail/aync_read_body.h>
#include <doodle_lib/render_farm/detail/computer.h>
#include <doodle_lib/render_farm/detail/post_log.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>
#include <doodle_lib/render_farm/detail/ue4_task.h>
#include <doodle_lib/render_farm/render_farm_fwd.h>
#include <doodle_lib/render_farm/work.h>
namespace doodle::render_farm {
namespace detail {

void render_job_type_post::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap)
    const {
  auto& l_session        = in_handle.get<working_machine_session>();
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
  auto l_parser_ptr      = std::make_shared<json_parser_type>(std::move(l_session.request_parser()));
  auto l_logger          = in_handle.get<socket_logger>().logger_;
  boost::beast::http::async_read(
      l_session.stream(), l_session.buffer(), *l_parser_ptr,
      [in_handle, l_parser_ptr, l_logger](boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        auto& l_session = in_handle.get<working_machine_session>();
        if (ec == boost::beast::http::error::end_of_stream) {
          return l_session.do_close();
        }
        if (ec) {
          log_error(l_logger, fmt::format("on_read error: {} ", ec));
          l_session.send_error_code(ec);
          return;
        }

        auto l_h = entt::handle{*g_reg(), g_reg()->create()};
        l_h.emplace<process_message>();
        try {
          l_h.emplace<ue4_task>(l_h, l_parser_ptr->release().body().get<ue4_task::arg_t>());
        } catch (const nlohmann::json::exception& e) {
          log_error(l_logger, fmt::format("json parse error:{} ", e.what()));
          l_session.send_error(e);
          return;
        }

        boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
        l_response.body() = {{"state", "ok"}, {"id", l_h.entity()}};
        l_response.keep_alive(l_parser_ptr->keep_alive());
        l_response.prepare_payload();
        l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
      }
  );
}

void computer_reg_type_post::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap)
    const {
  auto& l_session        = in_handle.get<working_machine_session>();
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
  auto l_parser_ptr      = std::make_shared<json_parser_type>(std::move(l_session.request_parser()));
  auto l_logger          = in_handle.get<socket_logger>().logger_;
  boost::beast::http::async_read(
      l_session.stream(), l_session.buffer(), *l_parser_ptr,
      [in_handle, l_parser_ptr, l_logger](boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        auto& l_session = in_handle.get<working_machine_session>();
        if (ec == boost::beast::http::error::end_of_stream) {
          return l_session.do_close();
        }
        if (ec) {
          log_error(l_logger, fmt::format("on_read error: {} ", ec));
          l_session.send_error_code(ec);
          return;
        }

        auto l_remote_ip = l_session.stream().socket().remote_endpoint().address().to_string();
        log_info(l_logger, fmt::format("computer_reg: {}", l_remote_ip));
        entt::handle l_handle{};
        auto l_p = l_session.url().params();

        if (auto l_it = l_p.find("id"); l_it != l_p.end()) {
          l_handle = entt::handle{*g_reg(), num_to_enum<entt::entity>(std::stoi((*l_it).value))};
        }
        if (!l_handle || !l_handle.all_of<computer>()) {
          try {
            g_reg()->view<computer>().each([&](const entt::entity& e, computer& in_computer) {
              if (in_computer.name() == l_remote_ip) {
                l_handle = entt::handle{*g_reg(), e};
              }
            });
          } catch (const nlohmann::json::exception& e) {
            log_info(l_logger, fmt::format("json parse error: {} ", e.what()));
            l_session.send_error(e);
            return;
          }
        }
        if (!l_handle || !l_handle.all_of<computer>()) {
          l_handle = entt::handle{*g_reg(), g_reg()->create()};
          l_handle.emplace<computer>().set_name(l_remote_ip);
        }
        if (auto l_it = l_p.find("status"); l_it != l_p.end()) {
          l_handle.get<computer>().delay((*l_it).value);
        } else {
          l_handle.get<computer>().delay();
        }
        boost::beast::http::response<basic_json_body> l_response{boost::beast::http::status::ok, 11};
        l_response.keep_alive(l_parser_ptr->keep_alive());
        l_response.body() = {{"state", "ok"}, {"id", l_handle.entity()}};

        log_info(
            l_logger, fmt::format("send computer_reg: {} response version: {}", l_session.url(), l_response.version())
        );

        l_response.prepare_payload();
        l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
      }
  );
}

void get_log_type_post::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap)
    const {
  using str_parser_type = boost::beast::http::request_parser<boost::beast::http::string_body>;
  auto& l_session       = in_handle.get<working_machine_session>();
  auto l_parser_ptr     = std::make_shared<str_parser_type>(std::move(l_session.request_parser()));
  auto l_h              = entt::handle{*g_reg(), num_to_enum<entt::entity>(std::stoi(in_cap.at("handle")))};

  boost::beast::http::async_read(
      l_session.stream(), l_session.buffer(), *l_parser_ptr,
      post_log{in_handle, l_h, post_log::log_enum::log, l_parser_ptr}
  );
}
void get_err_type_post::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap)
    const {
  using str_parser_type = boost::beast::http::request_parser<boost::beast::http::string_body>;
  auto& l_session       = in_handle.get<working_machine_session>();
  auto l_parser_ptr     = std::make_shared<str_parser_type>(std::move(l_session.request_parser()));
  auto l_h              = entt::handle{*g_reg(), num_to_enum<entt::entity>(std::stoi(in_cap.at("handle")))};

  boost::beast::http::async_read(
      l_session.stream(), l_session.buffer(), *l_parser_ptr,
      post_log{in_handle, l_h, post_log::log_enum::err, l_parser_ptr}
  );
}
}  // namespace detail
}  // namespace doodle::render_farm