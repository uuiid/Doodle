//
// Created by td_main on 2023/8/9.
//

#include "url_route_post.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>

#include <doodle_lib/render_farm/detail/aync_read_body.h>
#include <doodle_lib/render_farm/detail/computer.h>
#include <doodle_lib/render_farm/detail/post_log.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>
#include <doodle_lib/render_farm/detail/ue4_task.h>
#include <doodle_lib/render_farm/work.h>
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
        auto l_remote_ip = l_session.stream().socket().remote_endpoint().address().to_string();
        DOODLE_LOG_INFO("computer_reg: {}", l_remote_ip);
        entt::handle l_handle{};
        auto l_p = l_session.url().params();

        if (auto l_it = l_p.find("id"); l_it != l_p.end()) {
          l_handle = entt::handle{*g_reg(), num_to_enum<entt::entity>(std::stoi((*l_it).value))};
        }
        if (!l_handle) {
          try {
            g_reg()->view<computer>().each([&](const entt::entity& e, computer& in_computer) {
              if (in_computer.name() == l_remote_ip) {
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
        DOODLE_LOG_INFO("send computer_reg: {}", l_session.url().data());
        DOODLE_LOG_INFO("response version: {}", l_response.version());
        l_response.prepare_payload();
        l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
      }
  );
}
void run_job_post::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap) const {
  auto& l_session        = in_handle.get<working_machine_session>();
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
  auto l_parser_ptr      = std::make_shared<json_parser_type>(std::move(l_session.request_parser()));
  if (g_ctx().contains<render_farm::work_ptr>()) {
    g_ctx().get<render_farm::work_ptr>()->run_job(in_handle, in_cap);
  } else {
    boost::system::error_code ec{};
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::not_find_work_class);
    l_session.send_error_code(ec, boost::beast::http::status::internal_server_error);
  }
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