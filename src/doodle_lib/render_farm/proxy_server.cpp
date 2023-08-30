//
// Created by td_main on 2023/8/18.
//

#include "proxy_server.h"
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
    boost::signals2::scoped_connection connection_;
  };
  std::shared_ptr<data_type> ptr_;

 public:
  explicit proxy_server_session(boost::asio::ip::tcp::socket in_socket, boost::beast::tcp_stream& in_server_stream_)
      : ptr_(std::make_shared<data_type>(std::move(in_socket), in_server_stream_)) {}

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
        boost::beast::bind_front_handler(&proxy_server_session::do_write_header, shared_from_this())
    );
  }

  void do_write_header(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) {
      if (ec == boost::beast::http::error::end_of_stream) {
        return do_close();
      }
      DOODLE_LOG_ERROR("on_read error: {}", ec.message());
      return do_close();
    }
    ptr_->stream_.expires_after(10s);
    boost::beast::http::async_write_header(
        ptr_->server_stream_, ptr_->request_serializer_,
        boost::beast::bind_front_handler(&proxy_server_session::do_read_body, shared_from_this())
    );
  }

  void do_read_body(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) {
      if (ec == boost::beast::http::error::end_of_stream) {
        return do_close();
      }
      DOODLE_LOG_ERROR("on_read error: {}", ec.message());
      return do_close();
    }

    if (ptr_->request_parser_.is_done()) {
      do_server_read_header();
    }
    // 这里清除缓冲区
    ptr_->buffer_.clear();
    ptr_->stream_.expires_after(10s);
    boost::beast::http::async_read(
        ptr_->stream_, ptr_->buffer_, ptr_->request_parser_,
        boost::beast::bind_front_handler(&proxy_server_session::do_write_body, shared_from_this())
    );
  }

  void do_write_body(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) {
      if (ec == boost::beast::http::error::end_of_stream) {
        return do_close();
      }
      DOODLE_LOG_ERROR("on_read error: {}", ec.message());
      return do_close();
    }

    auto l_data                             = ptr_->buffer_.data();
    ptr_->request_parser_.get().body().data = l_data.data();
    ptr_->request_parser_.get().body().size = l_data.size();
    ptr_->request_parser_.get().body().more = !ptr_->request_parser_.is_done();

    ptr_->stream_.expires_after(10s);
    boost::beast::http::async_write(
        ptr_->server_stream_, ptr_->request_serializer_,
        boost::beast::bind_front_handler(&proxy_server_session::do_read_body, shared_from_this())
    );
  }

  void do_server_read_header() {
    ptr_->server_stream_.expires_after(10s);
    ptr_->buffer_.clear();
    boost::beast::http::async_read_header(
        ptr_->server_stream_, ptr_->buffer_, ptr_->response_parser_,
        boost::beast::bind_front_handler(&proxy_server_session::do_server_write_header, shared_from_this())
    );
  }

  void do_server_write_header(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) {
      if (ec == boost::beast::http::error::end_of_stream) {
        return do_close();
      }
      DOODLE_LOG_ERROR("on_read error: {}", ec.message());
      return do_close();
    }
    ptr_->server_stream_.expires_after(10s);
    boost::beast::http::async_write_header(
        ptr_->stream_, ptr_->response_serializer_,
        boost::beast::bind_front_handler(&proxy_server_session::do_server_read_body, shared_from_this())
    );
  }

  void do_server_read_body(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) {
      if (ec == boost::beast::http::error::end_of_stream) {
        return do_close();
      }
      DOODLE_LOG_ERROR("on_read error: {}", ec.message());
      return do_close();
    }

    if (ptr_->response_parser_.is_done()) {
      do_close();
    }
    // 这里清除缓冲区
    ptr_->buffer_.clear();
    ptr_->server_stream_.expires_after(10s);
    boost::beast::http::async_read(
        ptr_->server_stream_, ptr_->buffer_, ptr_->response_parser_,
        boost::beast::bind_front_handler(&proxy_server_session::do_server_write_body, shared_from_this())
    );
  }

  void do_server_write_body(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) {
      if (ec == boost::beast::http::error::end_of_stream) {
        return do_close();
      }
      DOODLE_LOG_ERROR("on_read error: {}", ec.message());
      return do_close();
    }

    auto l_data                              = ptr_->buffer_.data();
    ptr_->response_parser_.get().body().data = l_data.data();
    ptr_->response_parser_.get().body().size = l_data.size();
    ptr_->response_parser_.get().body().more = !ptr_->response_parser_.is_done();

    ptr_->server_stream_.expires_after(10s);
    boost::beast::http::async_write(
        ptr_->stream_, ptr_->response_serializer_,
        boost::beast::bind_front_handler(&proxy_server_session::do_server_read_body, shared_from_this())
    );
  }

  void do_close() { ptr_->stream_.socket().close(); }
};

void proxy_server::run() { do_accept(); }
void proxy_server::stop() {}
void proxy_server::do_accept() {
  acceptor_.async_accept(
      boost::asio::make_strand(g_io_context()), boost::beast::bind_front_handler(&proxy_server::on_accept, this)
  );
}
void proxy_server::on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
  if (ec) {
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }
    DOODLE_LOG_ERROR("on_accept error: {}", ec.what());
  } else {
    client_core_ptr_->async_connect(
        boost::asio::make_strand(g_io_context()),
        [this, socket = std::move(socket)](auto&& PH1, auto&& PH2) mutable {
          if (PH1) {
            DOODLE_LOG_ERROR("{}", PH1.message());
            return;
          }
          std::make_shared<proxy_server_session>(std::move(socket), *server_stream_)->run();
        }
    );
  }
  do_accept();
}
}  // namespace doodle