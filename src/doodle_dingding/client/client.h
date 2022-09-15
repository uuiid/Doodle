//
// Created by TD on 2022/9/7.
//

#pragma once

#include <doodle_dingding/doodle_dingding_fwd.h>
#include <doodle_core/exception/exception.h>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system.hpp>
#include <boost/url/urls.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast.hpp>
#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/doodle_core.h>
namespace doodle::dingding {
class client;

namespace client_ns {
template <typename Req, typename Res>
struct async_http_req_res_data {
  Req req_attr;
  Res res_attr;
  boost::url url_attr;
  boost::beast::flat_buffer buffer_;

  explicit async_http_req_res_data(
      Req in_req_attr,
      boost::url in_url_attr
  ) : req_attr(std::move(in_req_attr)),
      res_attr(),
      url_attr(std::move(in_url_attr)),
      buffer_() {
  }
};

template <typename Req, typename Res>
struct async_http_req_res {
 public:
  boost::beast::ssl_stream<boost::beast::tcp_stream>& ssl_stream;
  std::shared_ptr<client> self_attr;
  using data_ptr = std::shared_ptr<async_http_req_res_data<Req, Res>>;
  data_ptr p_data;

  enum {
    on_starting,

    on_resolve,
    on_connected,
    on_handshake,

    on_writing,
    on_reading,
  } state_;

  explicit async_http_req_res(
      Req in_req_attr,
      boost::url in_url_attr,
      boost::beast::ssl_stream<boost::beast::tcp_stream>& in_ssl_stream,
      std::shared_ptr<client> in_self_attr
  ) : ssl_stream(in_ssl_stream),
      self_attr(std::move(in_self_attr)),
      p_data(
          std::make_shared<data_ptr::element_type>(
              std::move(in_req_attr),
              std::move(in_url_attr)
          )
      ),
      state_(on_starting){};

  void write_prepare() {
    auto l_data = p_data;
    boost::url l_url{l_data->url_attr};
    l_url.remove_origin();  /// \brief 去除一部分
    /// \brief 设置http的一些通用方法
    l_data->req_attr.version(11);
    //    req_attr.method(boost::beast::http::verb::get);
    l_data->req_attr.target(l_url.c_str());
    l_data->req_attr.set(boost::beast::http::field::host, l_data->url_attr.host());
    l_data->req_attr.set(
        boost::beast::http::field::user_agent,
        BOOST_BEAST_VERSION_STRING
    );
    l_data->req_attr.prepare_payload();
  }

  template <typename Self>
  void operator()(
      Self& self,
      boost::system::error_code error = {},
      const Res& in_res               = {}
  ) {
    /// 检查错误
    if (error) {
      throw_exception(boost::system::system_error{error});
    }
    using namespace std::literals;
    boost::beast::get_lowest_layer(ssl_stream)
        .expires_after(30s);  /// 更新超时
    auto l_data = p_data;     /// 在这里复制一次内部数据, 放置移动时数据无法访问
    auto l_c    = self_attr;  /// 在这里复制一次客户端,放置客户端被移动时无法访问

    switch (state_) {
      case on_starting: {
        state_ = on_resolve;
        const std::string host{l_data->url_attr.host()};
        l_c->set_openssl(host);
        using namespace std::literals;
        const std::string port{l_data->url_attr.has_port()  //
                                   ? std::string{l_data->url_attr.port()}
                                   : "443"s};
        l_c->resolver().async_resolve(
            host,
            port,
            [self = std::move(self), l_c](
                boost::system::error_code ec,
                const boost::asio::ip::tcp::resolver::results_type& results
            ) mutable {
              l_c->results_attr = results;
              self(ec, Res{});
            }
        );
        break;
      }

      case on_resolve: {
        state_   = on_connected;
        ssl_stream.set_verify_mode(boost::asio::ssl::verify_peer);
        ssl_stream.set_verify_callback(
            [](bool preverified,
               boost::asio::ssl::verify_context& ctx) -> bool {
              char subject_name[256];
              X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
              X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
              DOODLE_LOG_INFO("Verifying {}", subject_name);

              return true;
            }
        );

        boost::beast::get_lowest_layer(ssl_stream)
            .async_connect(
                l_c->results_attr,
                [self = std::move(self)](
                    boost::system::error_code ec,
                    const boost::asio::ip::tcp::resolver::results_type::endpoint_type&
                ) mutable {
                  self(ec);
                }
            );
        break;
      }
      case on_connected: {
        state_ = on_handshake;
        ssl_stream.async_handshake(
            boost::asio::ssl::stream_base::client,
            [self = std::move(self)](
                boost::system::error_code ec
            ) mutable {
              self(ec);
            }
        );
        break;
      }
      case on_handshake: {
        state_ = on_writing;
        this->write_prepare();
        boost::beast::http::async_write(
            ssl_stream, l_data->req_attr,
            [l_self = std::move(self)](
                boost::system::error_code ec,
                std::size_t bytes_transferred
            ) mutable {
              boost::ignore_unused(bytes_transferred);
              l_self(ec);
            }
        );

        break;
      }

      case on_writing: {
        state_ = on_reading;
        boost::beast::http::async_read(
            ssl_stream,
            l_data->buffer_,
            l_data->res_attr,
            [l_self = std::move(self)](
                boost::system::error_code ec,
                std::size_t bytes_transferred
            ) mutable {
              boost::ignore_unused(bytes_transferred);
              l_self(ec);
            }
        );

        break;
      }
      case on_reading: {
        if (!l_data->res_attr.keep_alive())
          self_attr->async_shutdown();
        self.complete(error, l_data->res_attr);
        break;
      }
    }
  }
};

}  // namespace client_ns

/**
 * @brief 一个特别基础的https客户端
 * 使用方法 run("www.example.com","443","/")
 */
class DOODLE_DINGDING_API client
    : public std::enable_shared_from_this<client> {
 public:
  using executor_type = typename boost::asio::any_io_executor;

 protected:
  [[nodiscard("")]] boost::beast::ssl_stream<boost::beast::tcp_stream>& ssl_stream();

  class config_type_base {
   public:
    virtual void async_write(boost::beast::ssl_stream<boost::beast::tcp_stream>& in_stream) = 0;
    virtual void async_read(boost::beast::ssl_stream<boost::beast::tcp_stream>& in_stream)  = 0;
  };

  class config_type {
   public:
    std::function<void(boost::beast::ssl_stream<boost::beast::tcp_stream>&)> async_write;
    std::function<void(boost::beast::ssl_stream<boost::beast::tcp_stream>&)> async_read;
  };
  std::any other_data;

  class impl;
  std::unique_ptr<impl> ptr;

 public:
  void set_openssl(const std::string& host);
  bool is_connect();
  [[nodiscard("")]] boost::asio::ssl::context& ssl_context();
  [[nodiscard("")]] boost::asio::ip::tcp::resolver& resolver();
  boost::asio::ip::tcp::resolver::results_type results_attr{};

  void on_resolve(
      boost::system::error_code ec,
      const boost::asio::ip::tcp::resolver::results_type& results
  );

  void on_connect(
      boost::system::error_code ec,
      const boost::asio::ip::tcp::resolver::results_type::endpoint_type&
  );

  void on_handshake(boost::system::error_code ec);
  void on_write(
      boost::system::error_code ec,
      std::size_t bytes_transferred
  ) const;

  void on_shutdown(boost::system::error_code ec);

  void async_shutdown();

 public:
  explicit client(
      const boost::asio::any_io_executor& in_executor,
      boost::asio::ssl::context& in_ssl_context
  );

  void run(
      const std::string& in_host,
      const std::string& in_target
  );
  template <typename HttpReqResType>
  void run(HttpReqResType&& in) {
    other_data = std::move(in);
    config_type l_c{
        [this](boost::beast::ssl_stream<boost::beast::tcp_stream>& in) {
          std::any_cast<HttpReqResType>(this->other_data).async_write(in);
        },
        [this](boost::beast::ssl_stream<boost::beast::tcp_stream>& in) {
          std::any_cast<HttpReqResType>(this->other_data).async_read(in);
        }};
    return run(std::any_cast<HttpReqResType>(this->other_data).url_attr, l_c);
  }

  executor_type get_executor() noexcept;

  template <typename Response, typename CompletionToken, typename Request>
  auto async_write_read(
      Request in_request,
      boost::url in_url,
      CompletionToken&& in_token
  )
      -> decltype(boost::asio::async_compose<
                  CompletionToken, void(
                                       boost::system::error_code,
                                       const std::decay_t<Response>&
                                   )>(
          std::declval<client_ns::async_http_req_res<Request, Response>>(),
          in_token,
          ssl_stream()
      )) {
    using http_req_res = client_ns::async_http_req_res<Request, Response>;
    return boost::asio::async_compose<
        CompletionToken,
        void(
            boost::system::error_code,
            const std::decay_t<Response>&
        )>(
        http_req_res{
            std::move(in_request),
            std::move(in_url),
            ssl_stream(),
            shared_from_this()},
        in_token,
        ssl_stream()
    );
  };

 protected:
  /**
   * @brief 这个是主要的运行类,
   * @param in_url 传入的url, 用来分解 host, 和 post(post可也是默认 443 )
   * @param in_config_type 配置, 这个主要是用来指针函数对象, 在异步读写时需要
   */
  void run(
      const boost::url& in_url,
      const config_type& in_config_type
  );

 public:
  virtual ~client() noexcept;
};

namespace client_ns {
// template <class T>
// class base_http_req_res {
//  public:
//   void on_write(boost::system::error_code ec, std::size_t bytes_transferred) {
//     boost::ignore_unused(bytes_transferred);
//
//     if (ec) {
//       ;
//     }
//
//     // Receive the HTTP response
//     boost::beast::http::async_read(
//         ptr->ssl_stream, ptr->buffer_, ptr->res_,
//         boost::beast::bind_front_handler(&on_read, shared_from_this())
//     );
//   }
//   void on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
//     boost::ignore_unused(bytes_transferred);
//     if (ec) {
//       DOODLE_LOG_INFO("read {}", ec.what());
//       return;
//     }
//
//     // 打印消息
//     DOODLE_LOG_INFO(ptr->res_.body())
//
//     // Set a timeout on the operation
//     boost::beast::get_lowest_layer(ptr->ssl_stream)
//         .expires_after(30s);
//
//     // Gracefully close the stream
//     ptr->ssl_stream.async_shutdown(
//         boost::beast::bind_front_handler(
//             &client::on_shutdown,
//             shared_from_this()
//         )
//     );
//   }
// };
template <typename Req, typename Res>
class http_req_res {
  boost::beast::flat_buffer buffer_{};
  boost::beast::ssl_stream<boost::beast::tcp_stream>* ssl_stream{};

 public:
  explicit http_req_res(const std::shared_ptr<client>& in_self)
      : self(in_self) {}

  std::weak_ptr<client> self;
  Req req_attr;
  Res res_attr;
  boost::url url_attr;

  void async_write(
      boost::beast::ssl_stream<boost::beast::tcp_stream>& in_ssl_stream
  ) {
    boost::url l_url{url_attr};
    l_url.remove_origin();  /// \brief 去除一部分
    /// \brief 设置http的一些通用方法
    req_attr.version(11);
    //    req_attr.method(boost::beast::http::verb::get);
    req_attr.target(l_url.c_str());
    req_attr.set(boost::beast::http::field::host, url_attr.host());
    req_attr.set(
        boost::beast::http::field::user_agent,
        BOOST_BEAST_VERSION_STRING
    );
    req_attr.prepare_payload();

    boost::beast::http::async_write(
        in_ssl_stream, req_attr,
        boost::beast::bind_front_handler(
            &client::on_write, self.lock()->shared_from_this()
        )
    );
  }

  void async_read(
      boost::beast::ssl_stream<boost::beast::tcp_stream>& in_ssl_stream
  ) {
    ssl_stream = std::addressof(in_ssl_stream);
    // 接收HTTP响应
    boost::beast::http::async_read(
        in_ssl_stream, buffer_, res_attr,
        [self = self.lock()->shared_from_this(), this](auto&&... in) {
          this->on_read(std::forward<decltype(in)>(in)...);
        }
    );
  }
  void on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
    auto l_self = self.lock();
    boost::ignore_unused(bytes_transferred);
    if (ec) {
      throw_exception(boost::system::system_error{ec});
    }
    /// 设置超时
    using namespace std::literals;
    boost::beast::get_lowest_layer(*ssl_stream)
        .expires_after(30s);

    read_fun(res_attr);
    if (!res_attr.keep_alive())
      l_self->async_shutdown();
  }

  std::function<void(const Res&)> read_fun{};
};

}  // namespace client_ns
}  // namespace doodle::dingding
