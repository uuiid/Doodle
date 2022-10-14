//
// Created by TD on 2022/8/26.
//

#include "session.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
namespace doodle::http_server {

template <
    class Body,
    class Allocator,
    class Send>
void handle_request(
    boost::beast::string_view doc_root,
    boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>&& req,
    Send&& send
) {
  // Returns a bad request response
  auto const bad_request =
      [&req](boost::beast::string_view why) {
        boost::beast::http::response<
            boost::beast::http::string_body>
            res{boost::beast::http::status::bad_request, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
      };

  // Returns a not found response
  auto const not_found =
      [&req](boost::beast::string_view target) {
        boost::beast::http::response<boost::beast::http::string_body>
            res{boost::beast::http::status::not_found, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
      };

  // Returns a server error response
  auto const server_error =
      [&req](boost::beast::string_view what) {
        boost::beast::http::response<
            boost::beast::http::string_body>
            res{boost::beast::http::status::internal_server_error, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
      };

  // Make sure we can handle the method
  if (req.method() != boost::beast::http::verb::get &&
      req.method() != boost::beast::http::verb::head)
    return send(bad_request("Unknown HTTP-method"));

  // Request path must be absolute and not contain "..".
  if (req.target().empty() ||
      req.target()[0] != '/' ||
      req.target().find("..") != boost::beast::string_view::npos)
    return send(bad_request("Illegal request-target"));

  // Build the path to the requested file
  FSys::path path = std::string{req.target()};
  if (req.target().back() == '/')
    path.append("index.html");

  // Attempt to open the file
  boost::beast::error_code ec;
  boost::beast::http::file_body::value_type body;
  body.open(path.generic_string().c_str(), boost::beast::file_mode::scan, ec);

  // Handle the case where the file doesn't exist
  if (ec == boost::beast::errc::no_such_file_or_directory)
    return send(not_found(req.target()));

  // Handle an unknown error
  if (ec)
    return send(server_error(ec.message()));

  // Cache the size since we need it after the move
  auto const size = body.size();

  // Respond to HEAD request
  if (req.method() == boost::beast::http::verb::head) {
    boost::beast::http::response<boost::beast::http::empty_body>
        res{boost::beast::http::status::ok, req.version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, std::string{} /*mime_type(path)*/);
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
  }

  // Respond to GET request
  boost::beast::http::response<boost::beast::http::file_body> res{
      std::piecewise_construct,
      std::make_tuple(std::move(body)),
      std::make_tuple(boost::beast::http::status::ok, req.version())};
  res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(boost::beast::http::field::content_type, std::string{} /*mime_type(path)*/);
  res.content_length(size);
  res.keep_alive(req.keep_alive());
  return send(std::move(res));
}

class session::impl {
 public:
  explicit impl(boost::asio::ip::tcp::socket&& socket)
      : stream_(std::move(socket)) {}
  boost::beast::tcp_stream stream_;
  boost::beast::flat_buffer buffer_{};
  std::shared_ptr<std::string const> doc_root_{};
  boost::beast::http::request<boost::beast::http::string_body> req_{};
  std::shared_ptr<void> res_{};
};

session::session(boost::asio::ip::tcp::socket&& socket)
    : ptr(std::make_unique<impl>(std::move(socket))) {
}

void session::run() {
  // We need to be executing within a strand to perform async operations
  // on the I/O objects in this session. Although not strictly necessary
  // for single-threaded contexts, this example code is written to be
  // thread-safe by default.
  boost::asio::dispatch(
      ptr->stream_.get_executor(),
      boost::beast::bind_front_handler(
          &session::do_read,
          shared_from_this()
      )
  );
}
void session::do_read() {
  // Make the request empty before reading,
  // otherwise the operation behavior is undefined.
  ptr->req_ = {};

  // Set the timeout.
  ptr->stream_.expires_after(std::chrono::seconds(30));

  // Read a request
  boost::beast::http::async_read(
      ptr->stream_,
      ptr->buffer_,
      ptr->req_,
      boost::beast::bind_front_handler(&session::on_read, shared_from_this())
  );
}
void session::on_read(
    boost::beast::error_code ec, std::size_t bytes_transferred
) {
  boost::ignore_unused(bytes_transferred);

  // This means they closed the connection
  if (ec == boost::beast::http::error::end_of_stream)
    return do_close();

  if (ec)
    DOODLE_LOG_INFO("{}", ec);
  // Send the response
  handle_request(
      *ptr->doc_root_,
      std::move(ptr->req_),
      [this](auto&& msg) {
        // The lifetime of the message has to extend
        // for the duration of the async operation, so
        // we use a shared_ptr to manage it.
        auto sp         = std::make_shared<std::decay_t<decltype(msg)>>(std::forward<decltype(msg)>(msg));

        // Store a type-erased version of the shared
        // pointer in the class to keep it alive.
        this->ptr->res_ = sp;

        // Write the response
        boost::beast::http::async_write(
            this->ptr->stream_,
            *sp,
            boost::beast::bind_front_handler(
                &session::on_write,
                this->shared_from_this(),
                sp->need_eof()
            )
        );
      }
  );
}
void session::on_write(
    bool close, boost::beast::error_code ec, std::size_t bytes_transferred
) {
  boost::ignore_unused(bytes_transferred);

  if (ec)
    //    return fail(ec, "write");
    return;

  if (close) {
    // This means we should close the connection, usually because
    // the response indicated the "Connection: close" semantic.
    return do_close();
  }

  // We're done with the response so delete it
  ptr->res_ = nullptr;

  // Read another request
  do_read();
}
void session::do_close() {
  // Send a TCP shutdown
  ptr->stream_.socket().shutdown(tcp::socket::shutdown_send);
  // At this point the connection is closed gracefully
}
session::~session() = default;
}  // namespace doodle::http_server
