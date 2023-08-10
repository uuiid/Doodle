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

//  检查url

std::pair<boost::urls::segments_ref::iterator, boost::urls::segments_ref::iterator> chick_url(
    boost::urls::segments_ref in_segments_ref
) {
  auto l_begin = in_segments_ref.begin();
  auto l_end   = in_segments_ref.end();
  if (l_begin == l_end) {
    throw_exception(doodle_error{"url error"});
  }

  if (*l_begin != "v1") {
    throw_exception(doodle_error{"url version error"});
  }
  ++l_begin;
  if (l_begin == l_end) {
    throw_exception(doodle_error{"url not found render_farm"});
  }
  if (*l_begin != "render_farm") {
    throw_exception(doodle_error{"url not found render_farm"});
  }
  ++l_begin;
  if (l_begin == l_end) {
    throw_exception(doodle_error{"url not found action"});
  }
  return {l_begin, l_end};
}

// http 方法
template <boost::beast::http::verb in_method>
class http_method {
 public:
  void run(std::shared_ptr<working_machine_session> in_session) {
    boost::beast::http::response<boost::beast::http::empty_body> l_response{boost::beast::http::status::not_found, 11};
    in_session->send_response(boost::beast::http::message_generator{std::move(l_response)});
  };
};

template <>
class http_method<boost::beast::http::verb::get> {
  using map_actin_type = std::map<
      std::string, std::function<boost::beast::http::message_generator(const entt::handle&, boost::urls::params_ref)>>;
  const map_actin_type map_action;

 public:
  http_method()
      : map_action{
            {"get_log"s, [](const entt::handle& in_h, boost::urls::params_ref) { return get_log(in_h); }},
            {"get_err"s, [](const entt::handle& in_h, boost::urls::params_ref) { return get_err(in_h); }},
            {"render_job"s,
             [](const entt::handle& in_h, boost::urls::params_ref in_params) { return render_job(); }}} {}

  void run(std::shared_ptr<working_machine_session> in_session) {
    auto l_url                = boost::url{in_session->request_parser_.get().target()};

    auto [l_handle, l_method] = parser(chick_url(l_url.segments()));

    if (map_action.count(l_method) == 0) {
      boost::beast::http::response<boost::beast::http::empty_body> l_response{
          boost::beast::http::status::not_found, 11};
      in_session->send_response(boost::beast::http::message_generator{std::move(l_response)});
    } else {
      in_session->send_response(map_action.at(l_method)(l_handle, l_url.params()));
    }
  };

  static boost::beast::http::message_generator get_log(const entt::handle& in_h) {
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::ok, 11};
    l_response.body() = in_h.get<process_message>().log();
    return l_response;
  }

  static boost::beast::http::message_generator get_err(const entt::handle& in_h) {
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::ok, 11};
    l_response.body() = in_h.get<process_message>().err();
    return l_response;
  }
  static boost::beast::http::message_generator render_job() {
    auto l_view  = g_reg()->view<uuid>().each();
    auto l_uuids = l_view |
                   ranges::views::transform([](auto in_e) -> boost::uuids::uuid { return std::get<1>(in_e); }) |
                   ranges::to_vector;
    boost::beast::http::response<basic_json_body> l_response{boost::beast::http::status::ok, 11};
    l_response.body() = l_uuids;
    return {std::move(l_response)};
  }

  static std::tuple<entt::handle, std::string> parser(
      const std::pair<boost::urls::segments_ref::iterator, boost::urls::segments_ref::iterator>& in_segments
  ) {
    auto [l_begin, l_end] = in_segments;
    auto l_next           = l_begin++;
    entt::handle l_handle{*g_reg(), entt::null};
    if (l_next == l_end) {
      return {l_handle, *l_begin};
    } else {
      auto l_uuid = boost::lexical_cast<boost::uuids::uuid>(*l_begin);
      g_reg()->view<boost::uuids::uuid>().each([&](entt::entity in_entity, boost::uuids::uuid& in_uuid) {
        if (in_uuid == l_uuid) {
          l_handle = entt::handle{*g_reg(), in_entity};
        }
      });
      if (!l_handle) {
        throw_exception(doodle_error{"url not found id"});
      }

      ++l_begin;
      if (l_begin != l_end) {
        throw_exception(doodle_error{"url not found method"});
      }

      auto l_method = *l_begin;
      if (l_method.empty()) {
        throw_exception(doodle_error{" url method is empty"});
      }
      return {l_handle, l_method};
    }
  }
};

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
}
}  // namespace detail

void working_machine_session::run() {
  connection_ = doodle::app_base::Get().on_stop.connect([this]() { do_close(); });
  boost::asio::dispatch(
      boost::asio::make_strand(stream_.get_executor()),
      boost::beast::bind_front_handler(&working_machine_session::do_read, shared_from_this())
  );
}

template <boost::beast::http::verb http_verb>
void working_machine_session::do_parser() {
  stream_.expires_after(30s);
  std::make_shared<detail::http_method<http_verb>>()->run(shared_from_this());
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
      do_parser<boost::beast::http::verb::get>();
      break;
    case boost::beast::http::verb::head:
      do_parser<boost::beast::http::verb::head>();
      break;
    case boost::beast::http::verb::post: {
      do_parser<boost::beast::http::verb::post>();
      break;
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