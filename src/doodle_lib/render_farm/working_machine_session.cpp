//
// Created by td_main on 2023/8/3.
//

#include "working_machine_session.h"

#include <doodle_core/core/app_base.h>

#include <doodle_lib/render_farm/detail/render_ue4.h>

#include "boost/url/urls.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/url.hpp>
namespace doodle::render_farm {

namespace detail {
// template <typename Json_Conv_to>
struct basic_json_body {
 public:
  /** The type of container used for the body

      This determines the type of @ref message::body
      when this body type is used with a message container.
  */
  using json_type  = nlohmann::json;
  using value_type = json_type;

  /** Returns the payload size of the body

      When this body is used with @ref message::prepare_payload,
      the Content-Length will be set to the payload size, and
      any chunked Transfer-Encoding will be removed.
  */
  static std::uint64_t size(value_type const& body) { return body.size(); }

  /** The algorithm for parsing the body

      Meets the requirements of <em>BodyReader</em>.
  */

  class reader {
    value_type& body_;
    std::string json_str_;

   public:
    template <bool isRequest, class Fields>
    explicit reader(boost::beast::http::header<isRequest, Fields>&, value_type& b) : body_(b) {}

    void init(boost::optional<std::uint64_t> const& length, boost::system::error_code& ec) {
      if (length) {
        if (*length > json_str_.max_size()) {
          BOOST_BEAST_ASSIGN_EC(ec, boost::beast::http::error::buffer_overflow);
          return;
        }
        json_str_.reserve(boost::beast::detail::clamp(*length));
      }
      ec = {};
    }

    template <class ConstBufferSequence>
    std::size_t put(ConstBufferSequence const& buffers, boost::system::error_code& ec) {
      auto const extra = boost::beast::buffer_bytes(buffers);
      auto const size  = json_str_.size();
      if (extra > json_str_.max_size() - size) {
        BOOST_BEAST_ASSIGN_EC(ec, boost::beast::http::error::buffer_overflow);
        return 0;
      }

      json_str_.resize(size + extra);
      ec         = {};
      char* dest = &json_str_[size];
      for (auto b : boost::beast::buffers_range_ref(buffers)) {
        std::char_traits<char>::copy(dest, static_cast<char const*>(b.data()), b.size());
        dest += b.size();
      }
      return extra;
    }

    void finish(boost::system::error_code& ec) {
      ec    = {};
      body_ = json_type::parse(json_str_);
    }
  };

  /** The algorithm for serializing the body

      Meets the requirements of <em>BodyWriter</em>.
  */

  class writer {
    value_type const& body_;
    std::string json_str_;

   public:
    using const_buffers_type = boost::asio::const_buffer;

    template <bool isRequest, class Fields>
    explicit writer(boost::beast::http::header<isRequest, Fields> const&, value_type const& b) : body_(b) {}

    void init(boost::system::error_code& ec) {
      json_str_ = body_.dump();
      ec        = {};
    }

    boost::optional<std::pair<const_buffers_type, bool>> get(boost::system::error_code& ec) {
      ec = {};
      return {{const_buffers_type{json_str_.data(), json_str_.size()}, false}};
    }
  };
};
}  // namespace detail

void working_machine_session::run() {
  connection_ = doodle::app_base::Get().on_stop.connect([this]() { do_close(); });
  boost::asio::dispatch(
      boost::asio::make_strand(stream_.get_executor()),
      boost::beast::bind_front_handler(&working_machine_session::do_read, shared_from_this())
  );
}

void working_machine_session::do_read() {
  stream_.expires_after(30s);
  boost::beast::http::async_read_header(
      stream_, buffer_, request_parser_,
      boost::beast::bind_front_handler(&working_machine_session::on_parser, shared_from_this())
  );
}
void working_machine_session::on_parser(boost::system::error_code ec, std::size_t bytes_transferred) {
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;

  switch (request_parser_.get().method()) {
    case boost::beast::http::verb::get: {
      auto l_url      = boost::url{request_parser_.get().target()};
      auto l_segments = l_url.segments();
      for (auto l_begin = l_segments.begin(); l_begin != l_segments.end(); ++l_begin) {
        if (*l_begin != "v1") break;

        if (++l_begin == l_segments.end()) break;
        if (*l_begin != "render_frame") break;

        if (++l_begin == l_segments.end()) break;
        auto l_uuid = boost::lexical_cast<uuid>(*l_begin);

        if (++l_begin == l_segments.end()) break;
        // 方法
        auto l_uuid_view = g_reg()->view<uuid>().each();
        if (auto l_it = ranges::find_if(l_uuid_view, [&](auto&& v) { return std::get<1>(v) == l_uuid; });
            l_it != l_uuid_view.end()) {
          auto l_method = *l_begin;
          auto l_h      = entt::handle{*g_reg(), std::get<0>(*l_it)};
          if (l_method == "get_state") {
            auto l_state = l_h.get<process_message>().get_state();
            boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
            l_response.body() = {{"state", l_state}, {"uuid", l_uuid}};
            send_response(boost::beast::http::message_generator{std::move(l_response)});
            return;
          } else if (l_method == "get_log") {
            std::string_view l_log{};
            if (auto l_params = l_url.params(); l_url.has_query() && l_params.contains("begin")) {
              auto l_bit = l_params.find("begin"), l_eit = l_params.find("end");
              std::size_t ll_begin = std::stoll((*l_bit).value),
                          ll_end   = l_eit != l_params.end() ? std::stoll((*l_eit).value) : -1;
              l_log                = l_h.get<process_message>().log(ll_begin, ll_end);
            } else {
              l_log = l_h.get<process_message>().log();
            }
            boost::beast::http::response<boost::beast::http::string_body> l_response{
                boost::beast::http::status::ok, 11};
            l_response.body() = l_log;
            send_response(boost::beast::http::message_generator{std::move(l_response)});
            return;
          } else if (l_method == "get_err") {
            std::string_view l_err{};
            if (auto l_params = l_url.params(); l_url.has_query() && l_params.contains("begin")) {
              auto l_bit = l_params.find("begin"), l_eit = l_params.find("end");
              std::size_t ll_begin = std::stoll((*l_bit).value),
                          ll_end   = l_eit != l_params.end() ? std::stoll((*l_eit).value) : -1;
              l_err                = l_h.get<process_message>().err(ll_begin, ll_end);
            } else {
              l_err = l_h.get<process_message>().err();
            }
            boost::beast::http::response<boost::beast::http::string_body> l_response{
                boost::beast::http::status::ok, 11};
            l_response.body() = l_err;
            send_response(boost::beast::http::message_generator{std::move(l_response)});
            return;
          } else if (l_method == "get_time") {
            auto l_time = l_h.get<process_message>().get_time();
            boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
            l_response.body() = {{"time", l_time}, {"uuid", l_uuid}};
            send_response(boost::beast::http::message_generator{std::move(l_response)});
            return;
          }
        }
      }

      boost::beast::http::response<boost::beast::http::empty_body> l_response{
          boost::beast::http::status::not_found, 11};
      send_response(boost::beast::http::message_generator{std::move(l_response)});
      break;
    }
    case boost::beast::http::verb::head: {
      boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
      l_response.body() = {{"hello", "world"}};
      send_response(boost::beast::http::message_generator{std::move(l_response)});
      break;
    }

    case boost::beast::http::verb::post: {
      if (request_parser_.get().target() == "/v1/render_frame/submit_job") {
        auto l_parser_ptr = std::make_shared<json_parser_type>(std::move(request_parser_));
        boost::beast::http::async_read(
            stream_, buffer_, *l_parser_ptr,
            [this, self = shared_from_this(),
             l_parser_ptr](boost::system::error_code ec, std::size_t bytes_transferred) {
              boost::ignore_unused(bytes_transferred);
              if (ec == boost::beast::http::error::end_of_stream) {
                return do_close();
              }
              if (ec) {
                DOODLE_LOG_ERROR("on_read error: {}", ec.message());
                return;
              }

              auto l_h = entt::handle{*g_reg(), g_reg()->create()};
              l_h.emplace<process_message>();
              l_h.emplace<uuid>();
              l_h
                  .emplace<render_ue4_ptr>(std::make_shared<render_ue4_ptr ::element_type>(
                      l_h, l_parser_ptr->release().body().get<render_ue4_ptr::element_type::arg>()
                  ))
                  ->run();

              boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
              l_response.body() = {{"state", "ok"}, {"uuid", l_h.get<uuid>()}};
              l_response.keep_alive(false);
              send_response(boost::beast::http::message_generator{std::move(l_response)});
            }
        );
        break;
      }
      // 如果不是提交任务, 直接返回失败
    }
    default: {
      boost::beast::http::response<boost::beast::http::empty_body> l_response{
          boost::beast::http::status::not_found, 11};
      send_response(boost::beast::http::message_generator{std::move(l_response)});
      break;
    }
  }
}

void working_machine_session::send_response(boost::beast::http::message_generator&& in_message_generator) {
  const bool keep_alive = in_message_generator.keep_alive();

  boost::beast::async_write(
      stream_, std::move(in_message_generator),
      [this, self = shared_from_this(), keep_alive](boost::system::error_code ec, std::size_t bytes_transferred) {
        on_write(keep_alive, ec, bytes_transferred);
      }
  );
}
void working_machine_session::on_write(bool keep_alive, boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec) {
    DOODLE_LOG_ERROR("on_write error: {}", ec.message());
    return;
  }

  if (!keep_alive) {
    return do_close();
  }

  do_read();
}
void working_machine_session::do_close() {
  boost::system::error_code ec;
  stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
}

}  // namespace doodle::render_farm