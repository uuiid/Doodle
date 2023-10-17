//
// Created by td_main on 2023/8/18.
//

#include "proxy_server.h"

#include <doodle_core/lib_warp/boost_fmt_asio.h>
namespace doodle {

class proxy_server_session : public std::enable_shared_from_this<proxy_server_session> {
 private:
  //  template <bool Is_Request>
  //  struct http_data : public std::enable_shared_from_this<http_data<Is_Request>> {
  //    boost::beast::http::parser<Is_Request, boost::beast::http::buffer_body> parser_;
  //    boost::beast::http::serializer<Is_Request, boost::beast::http::buffer_body> serializer_{parser_.get()};
  //
  //    template <typename SyncWriteStream, typename SyncReadStream, typename DynamicBuffer>
  //    void relay(SyncWriteStream& in_out_stream, SyncReadStream& in_int_stream, DynamicBuffer& in_buffer) {
  //      in_out_stream.expires_after(10s);
  //      in_int_stream.expires_after(10s);
  //      boost::beast::http::async_read_header(
  //          in_int_stream, in_buffer, parser_,
  //          boost::beast::bind_front_handler(&proxy_server_session::do_write_header, shared_from_this())
  //      );
  //    }
  //  };

  struct data_type {
    data_type(boost::asio::ip::tcp::socket in_socket, boost::beast::tcp_stream& in_server_stream_)
        : stream_(std::move(in_socket)), server_stream_(in_server_stream_) {}
    boost::beast::tcp_stream stream_;
    boost::beast::tcp_stream& server_stream_;
    boost::beast::http::request_parser<boost::beast::http::buffer_body> request_parser_;
    boost::beast::http::request_serializer<boost::beast::http::buffer_body> request_serializer_{request_parser_.get()};

    boost::beast::http::response_parser<boost::beast::http::buffer_body> response_parser_;
    boost::beast::http::response_serializer<boost::beast::http::buffer_body> response_serializer_{
        response_parser_.get()};

    boost::beast::flat_buffer buffer_;
    boost::beast::flat_buffer buffer_req;
    boost::beast::flat_buffer buffer_res;
    //    boost::signals2::scoped_connection connection_;

    proxy_server* server_ptr_{};
  };
  std::shared_ptr<data_type> ptr_;
  template <bool isRequest>
  struct relay_t {
    // Create a parser with a buffer body to read from the input.
    boost::beast::http::parser<isRequest, boost::beast::http::buffer_body> p;

    // Create a serializer from the message contained in the parser.
    boost::beast::http::serializer<isRequest, boost::beast::http::buffer_body, boost::beast::http::fields> sr{p.get()};

    // A small buffer for relaying the body piece by piece
    char buf[2048];

    proxy_server* server_ptr_{};

    template <class SyncWriteStream, class SyncReadStream, class DynamicBuffer>
    void relay_handle(
        SyncWriteStream& output, SyncReadStream& input, DynamicBuffer& buffer, boost::beast::error_code& ec
    ) {
      static_assert(boost::beast::is_sync_write_stream<SyncWriteStream>::value, "SyncWriteStream requirements not met");

      static_assert(boost::beast::is_sync_read_stream<SyncReadStream>::value, "SyncReadStream requirements not met");

      // Read just the header from the input
      boost::beast::http::read_header(input, buffer, p, ec);
      if (ec) {
        return;
      }

      // Apply the caller's header transformation
      //    transform(p.get(), ec);
      if (ec) {
        return;
      }

      // Send the transformed message to the output
      boost::beast::http::write_header(output, sr, ec);
      if (ec) {
        if ((ec == boost::beast::errc::not_connected || ec == boost::beast::errc::connection_reset ||
             ec == boost::beast::errc::connection_refused || ec == boost::beast::errc::connection_aborted) &&
            isRequest) {
          server_ptr_->do_connect_sync();
          boost::beast::http::write_header(output, sr, ec);
        } else {
          return;
        }
      }
    }
    template <class SyncWriteStream, class SyncReadStream, class DynamicBuffer>
    void relay_body(
        SyncWriteStream& output, SyncReadStream& input, DynamicBuffer& buffer, boost::beast::error_code& ec
    ) {
      static_assert(boost::beast::is_sync_write_stream<SyncWriteStream>::value, "SyncWriteStream requirements not met");

      static_assert(boost::beast::is_sync_read_stream<SyncReadStream>::value, "SyncReadStream requirements not met");

      // Loop over the input and transfer it to the output
      do {
        if (!p.is_done()) {
          // Set up the body for writing into our small buffer
          p.get().body().data = buf;
          p.get().body().size = sizeof(buf);

          // Read as much as we can
          boost::beast::http::read(input, buffer, p, ec);

          // This error is returned when buffer_body uses up the buffer
          if (ec == boost::beast::http::error::need_buffer) ec = {};
          if (ec) {
            return;
          }

          // Set up the body for reading.
          // This is how much was parsed:
          p.get().body().size = sizeof(buf) - p.get().body().size;
          p.get().body().data = buf;
          p.get().body().more = !p.is_done();
        } else {
          p.get().body().data = nullptr;
          p.get().body().size = 0;
          p.get().body().more = !p.is_done();
        }

        // Write everything in the buffer (which might be empty)
        boost::beast::http::write(output, sr, ec);

        // This error is returned when buffer_body uses up the buffer
        if (ec == boost::beast::http::error::need_buffer) {
          ec = {};
        }
        if (ec) {
          return;
        }
      } while (!p.is_done() && !sr.is_done());
    }
  };

 public:
  explicit proxy_server_session(
      boost::asio::ip::tcp::socket in_socket, boost::beast::tcp_stream& in_server_stream_, proxy_server* in_server_ptr
  )
      : ptr_(std::make_shared<data_type>(std::move(in_socket), in_server_stream_)) {
    ptr_->server_ptr_ = in_server_ptr;
  }

  void run2() {
    boost::beast::error_code ec;
    relay_t<true> l_relay_req{};
    l_relay_req.server_ptr_ = ptr_->server_ptr_;

    relay_t<false> l_relay_res{};
    l_relay_res.server_ptr_ = ptr_->server_ptr_;

    l_relay_req.relay_handle(ptr_->server_stream_, ptr_->stream_, ptr_->buffer_req, ec);
    if (ec) {
      DOODLE_LOG_INFO("{}", ec);
      return;
    }
    l_relay_req.relay_body(ptr_->server_stream_, ptr_->stream_, ptr_->buffer_req, ec);
    if (ec) {
      DOODLE_LOG_INFO("{}", ec);
      return;
    }
    l_relay_res.relay_handle(ptr_->stream_, ptr_->server_stream_, ptr_->buffer_res, ec);
    if (ec) {
      DOODLE_LOG_INFO("{}", ec);
      return;
    }
    l_relay_res.relay_body(ptr_->stream_, ptr_->server_stream_, ptr_->buffer_res, ec);
    if (ec) {
      DOODLE_LOG_INFO("{}", ec);
      return;
    }
  }

  void run() {
    boost::asio::dispatch(
        boost::asio::make_strand(ptr_->stream_.get_executor()),
        boost::beast::bind_front_handler(&proxy_server_session::do_read_header, shared_from_this())
    );
  }
  void do_read_header() {
    ptr_->stream_.expires_after(10s);
    boost::beast::http::async_read_header(
        ptr_->stream_, ptr_->buffer_, ptr_->request_parser_,
        [this, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
          if (ec) {
            //                      if (ec == boost::beast::http::error::end_of_stream) {
            //                        return do_close();
            //                      }
            DOODLE_LOG_ERROR("on_read error: {}", ec.message());
            //                      return do_close();
          }
          DOODLE_LOG_INFO("do_read_header {}", ptr_->request_parser_.get().target());
          ptr_->stream_.expires_after(10s);
          do_write_header();
        }
    );
  }
  void do_connect() {
    ptr_->server_ptr_->do_connect();
    ptr_->server_stream_.expires_after(10s);
    DOODLE_LOG_INFO("do_connect {}", ptr_->server_ptr_->server_address_);

    auto l_timer = std::make_shared<boost::asio::system_timer>(g_io_context());
    l_timer->expires_after(1s);
    l_timer->async_wait([l_timer, self = shared_from_this()](auto&& PH1) {
      if (PH1) {
        DOODLE_LOG_ERROR("timer error: {}", PH1.message());
        return;
      }
      DOODLE_LOG_INFO("timer success");
      self->do_write_header();
    });
  }

  void do_write_header() {
    ptr_->stream_.expires_after(10s);
    boost::beast::http::async_write_header(
        ptr_->server_stream_, ptr_->request_serializer_,
        [this, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
          if (ec) {
            if (ec == boost::beast::http::error::end_of_stream) {
              return do_close();
            }
            if (ec == boost::beast::errc::not_connected || ec == boost::beast::errc::connection_reset ||
                ec == boost::beast::errc::connection_refused || ec == boost::beast::errc::connection_aborted) {
              do_connect();
              return;
            }
            DOODLE_LOG_ERROR("on_read error: {}", ec.message());
            return do_close();
          }
          DOODLE_LOG_INFO("do_write_header {}", ptr_->request_parser_.get().target());
          ptr_->stream_.expires_after(10s);
          do_read_body();
        }
    );
  }

  void do_read_body() {
    if (ptr_->request_parser_.is_done()) {
      DOODLE_LOG_INFO("body is done");
      do_write_body();
      return;
    }
    // 这里清除缓冲区
    ptr_->buffer_.clear();
    ptr_->stream_.expires_after(10s);
    boost::beast::http::async_read(
        ptr_->stream_, ptr_->buffer_, ptr_->request_parser_,
        [this, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
          if (ec) {
            if (ec == boost::beast::http::error::end_of_stream) {
              return do_close();
            }
            DOODLE_LOG_ERROR("on_read error: {}", ec.message());
            return do_close();
          }
          DOODLE_LOG_INFO("do_read_body {}", ptr_->request_parser_.get().target());
          ptr_->stream_.expires_after(10s);
          do_write_body();
        }
    );
  }

  void do_write_body() {
    auto l_data                             = ptr_->buffer_.data();
    ptr_->request_parser_.get().body().data = l_data.data();
    ptr_->request_parser_.get().body().size = l_data.size();
    ptr_->request_parser_.get().body().more = !ptr_->request_parser_.is_done();

    ptr_->stream_.expires_after(10s);
    boost::beast::http::async_write(
        ptr_->server_stream_, ptr_->request_serializer_,
        [this, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
          if (ec) {
            if (ec == boost::beast::http::error::end_of_stream) {
              return do_close();
            }
            DOODLE_LOG_ERROR("on_read error: {}", ec.message());
            return do_close();
          }
          DOODLE_LOG_INFO("do_write_body {}", ptr_->request_parser_.get().target());
          ptr_->stream_.expires_after(10s);
          if (ptr_->request_parser_.is_done() || ptr_->request_serializer_.is_done()) {
            do_server_read_header();
          }

          do_read_body();
        }
    );
  }

  void do_server_read_header() {
    ptr_->server_stream_.expires_after(10s);
    ptr_->buffer_.clear();
    boost::beast::http::async_read_header(
        ptr_->server_stream_, ptr_->buffer_, ptr_->response_parser_,
        [this, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
          //          if (ec == boost::beast::http::error::end_of_stream) {
          //            do_server_write_header();
          //          }
          //          if (ec) {
          //            DOODLE_LOG_ERROR("on_read error: {}", ec.message());
          //            return do_close();
          //          }
          DOODLE_LOG_INFO("do_server_read_header {}", ptr_->response_parser_.get().body().size);
          ptr_->server_stream_.expires_after(10s);
          do_server_write_header();
        }
    );
  }

  void do_server_write_header() {
    boost::beast::http::async_write_header(
        ptr_->stream_, ptr_->response_serializer_,
        [this, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
          if (ec) {
            if (ec == boost::beast::http::error::end_of_stream) {
              return do_close();
            }
            DOODLE_LOG_ERROR("on_read error: {}", ec.message());
            return do_close();
          }
          DOODLE_LOG_INFO("do_server_write_header {}", ptr_->response_parser_.get().body().size);
          ptr_->server_stream_.expires_after(10s);
          do_server_read_body();
        }
    );
  }

  void do_server_read_body() {
    if (ptr_->response_parser_.is_done()) {
      DOODLE_LOG_INFO("body is done");
      do_server_write_body();
    }
    // 这里清除缓冲区
    ptr_->buffer_.clear();
    ptr_->server_stream_.expires_after(10s);
    boost::beast::http::async_read(
        ptr_->server_stream_, ptr_->buffer_, ptr_->response_parser_,
        [this, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
          if (ec) {
            if (ec == boost::beast::http::error::end_of_stream) {
              return do_close();
            }
            DOODLE_LOG_ERROR("on_read error: {}", ec.message());
            return do_close();
          }
          DOODLE_LOG_INFO("do_server_read_body {}", ptr_->response_parser_.get().body().size);
          ptr_->server_stream_.expires_after(10s);
          do_server_write_body();
        }
    );
  }

  void do_server_write_body() {
    auto l_data                              = ptr_->buffer_.data();
    ptr_->response_parser_.get().body().data = l_data.data();
    ptr_->response_parser_.get().body().size = l_data.size();
    ptr_->response_parser_.get().body().more = !ptr_->response_parser_.is_done();

    ptr_->server_stream_.expires_after(10s);
    boost::beast::http::async_write(
        ptr_->stream_, ptr_->response_serializer_,
        [this, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
          if (ec) {
            if (ec == boost::beast::http::error::end_of_stream) {
              return do_close();
            }
            DOODLE_LOG_ERROR("on_read error: {}", ec.message());
            return do_close();
          }
          DOODLE_LOG_INFO("do_server_write_body {}", ptr_->response_parser_.get().body().size);
          ptr_->server_stream_.expires_after(10s);
          if (ptr_->response_parser_.is_done() || ptr_->response_serializer_.is_done()) {
            do_read_header();
          }
          do_server_read_body();
        }
    );
  }

  void do_close() { ptr_->stream_.socket().close(); }
};

void proxy_server::run() {
  server_stream_ = std::make_shared<stream_t>(g_io_context());
  resolver_      = std::make_shared<resolver_t>(g_io_context());
  do_resolve();
  do_accept();
}
void proxy_server::stop() { acceptor_.cancel(); }
void proxy_server::do_resolve() {
  resolver_->async_resolve(server_address_, server_port_, [this](auto&& PH1, auto&& PH2) {
    if (PH1) {
      DOODLE_LOG_ERROR("resolver_ error: {}", PH1.message());
      return;
    }
    resolver_results_ = std::move(PH2);
    DOODLE_LOG_INFO("resolver_ success");
    //    do_connect();
  });
}
void proxy_server::do_connect() {
  boost::asio::async_connect(server_stream_->socket(), resolver_results_, [](auto&& PH1, auto&& PH2) {
    if (PH1) {
      DOODLE_LOG_ERROR("async_connect error: {}", PH1.message());
      return;
    }
    DOODLE_LOG_INFO("async_connect success");
  });
}

void proxy_server::do_accept() {
  acceptor_.async_accept(
      boost::asio::make_strand(g_io_context()),
      boost::beast::bind_front_handler(&proxy_server::on_accept, shared_from_this())
  );
}
void proxy_server::on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
  if (ec) {
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }
    DOODLE_LOG_ERROR("on_accept error: {}", ec.what());
  } else {
    do_connect_sync();
    std::make_shared<proxy_server_session>(std::move(socket), *server_stream_, this)->run2();
  }
  do_accept();
}
void proxy_server::do_connect_sync() { boost::asio::connect(server_stream_->socket(), resolver_results_); }
}  // namespace doodle