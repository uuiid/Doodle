//
// Created by td_main on 2023/8/3.
//

#include "working_machine_session.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
namespace doodle::render_farm {

template <class Body, class Allocator>
boost::beast::http::message_generator handle_request(
    boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>&& req
) {
  // Returns a bad request response
  auto const bad_request = [&req](boost::beast::string_view why) {
    boost::beast::http::response<boost::beast::http::string_body> res{
        boost::beast::http::status::bad_request, req.version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(why);
    res.prepare_payload();
    return res;
  };

  // Returns a not found response
  auto const not_found = [&req](boost::beast::string_view target) {
    boost::beast::http::response<boost::beast::http::string_body> res{
        boost::beast::http::status::not_found, req.version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + std::string(target) + "' was not found.";
    res.prepare_payload();
    return res;
  };

  // Returns a server error response
  auto const server_error = [&req](boost::beast::string_view what) {
    boost::beast::http::response<boost::beast::http::string_body> res{
        boost::beast::http::status::internal_server_error, req.version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + std::string(what) + "'";
    res.prepare_payload();
    return res;
  };

  // Attempt to open the file
  boost::beast::error_code ec;
  boost::beast::http::string_body l_body;
  ;

  // Respond to HEAD request
  if (req.method() == boost::beast::http::verb::head) {
    boost::beast::http::response<boost::beast::http::empty_body> res{boost::beast::http::status::ok, req.version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
  }

  // Respond to GET request
  boost::beast::http::response<boost::beast::http::file_body> res{
      std::piecewise_construct, std::make_tuple(std::move(body)),
      std::make_tuple(boost::beast::http::status::ok, req.version())};
  res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(boost::beast::http::field::content_type, mime_type(path));
  res.content_length(size);
  res.keep_alive(req.keep_alive());
  return res;
}

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
      try {
        body_ = json_type::parse(json_str_);
      } catch (json_type::exception const& e) {
        //        BOOST_BEAST_ASSIGN_EC(ec, boost::beast::http::error::body_parse_error);
        throw_exception(doodle_error{e.what()});
        return;
      }
      ec = {};
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
  switch (request_parser_.get().method()) {
    case boost::beast::http::verb::get:
      break;
    case boost::beast::http::verb::head:
      break;

    case boost::beast::http::verb::post: {
      if (request_parser_.get().target() == "/v1/render_frame/submit_job") {
        using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
        auto l_parser_ptr      = std::make_shared<json_parser_type>(std::move(request_parser_));
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
              send_response(boost::beast::http::message_generator{});
            }
        );
        break;
      }
      default:
        break;
    }
  }

  void working_machine_session::on_read(boost::system::error_code ec, std::size_t bytes_transferred) {}
  void working_machine_session::send_response(boost::beast::http::message_generator && in_message_generator) {
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

    request_ = {};
    do_read();
  }
  void working_machine_session::do_close() {
    boost::system::error_code ec;
    stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
  }

}  // namespace doodle::render_farm