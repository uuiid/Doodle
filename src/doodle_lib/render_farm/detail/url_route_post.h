//
// Created by td_main on 2023/8/9.
//

#pragma once
#include <doodle_lib/render_farm/detail/basic_json_body.h>
#include <doodle_lib/render_farm/working_machine_session.h>

#include <boost/beast.hpp>
#include <boost/url.hpp>

#include <nlohmann/json.hpp>
namespace doodle::render_farm {
namespace detail {

template <>
class http_method<boost::beast::http::verb::post> {
  using map_action_type =
      std::map<std::string, std::function<void(std::shared_ptr<working_machine_session>, boost::urls::params_ref)>>;
  const map_action_type map_action;

 public:
  http_method()
      : map_action{{"render_job"s, [](std::shared_ptr<working_machine_session> in_session, boost::urls::params_ref) {
                      return render_job(in_session);
                    }}} {}
  void run(std::shared_ptr<working_machine_session> in_session) {
    auto l_url = boost::url{in_session->request_parser_.get().target()};

    auto l_m   = parser(chick_url(l_url.segments()));

    if (map_action.count(l_m) == 0) {
      boost::beast::http::response<boost::beast::http::empty_body> l_response{
          boost::beast::http::status::not_found, 11};
      in_session->send_response(boost::beast::http::message_generator{std::move(l_response)});
    } else {
      map_action.at(l_m)(in_session, l_url.params());
    }
  };

  static std::string parser(
      const std::pair<boost::urls::segments_ref::iterator, boost::urls::segments_ref::iterator>& in_segments
  ) {
    auto [l_begin, l_end] = in_segments;
    return *l_begin;
  }

  static void render_job(std::shared_ptr<working_machine_session> in_session) {
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
            return;
          }

          auto l_h = entt::handle{*g_reg(), g_reg()->create()};
          l_h.emplace<process_message>();
          auto& l_uuid = l_h.emplace<uuid>();
          l_h.emplace<render_ue4_ptr>(std::make_shared<render_ue4_ptr ::element_type>(
                                          l_h, l_parser_ptr->release().body().get<render_ue4_ptr::element_type::arg>()
                                      ))
              ->run();

          boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
          l_response.body() = {{"state", "ok"}, {"uuid", l_uuid}};
          l_response.keep_alive(false);
          self->send_response(boost::beast::http::message_generator{std::move(l_response)});
        }
    );
  }
};
}  // namespace detail
}  // namespace doodle::render_farm
