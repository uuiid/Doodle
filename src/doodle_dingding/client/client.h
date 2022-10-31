//
// Created by TD on 2022/9/7.
//

#pragma once

#include <doodle_core/doodle_core.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/entt_warp.h>

#include <doodle_dingding/doodle_dingding_fwd.h>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/system.hpp>
#include <boost/url/urls.hpp>
namespace doodle::dingding {
class client;

namespace client_ns {
template <typename Req, typename Res>
struct async_http_req_res_data {
  Req req_attr;
  Res res_attr;
  boost::url url_attr;
  boost::beast::flat_buffer buffer_;
  boost::asio::ip::tcp::resolver::results_type results_attr{};

  explicit async_http_req_res_data(Req in_req_attr, boost::url in_url_attr)
      : req_attr(std::move(in_req_attr)), res_attr(), url_attr(std::move(in_url_attr)), buffer_() {}
};

template <typename Req, typename Res>
struct async_http_req_res {
 public:
  boost::beast::ssl_stream<boost::beast::tcp_stream>& ssl_stream;
  std::shared_ptr<client> self_attr;
  using data_type = async_http_req_res_data<Req, Res>;
  using data_ptr  = std::shared_ptr<data_type>;
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
      Req in_req_attr, boost::url in_url_attr, boost::beast::ssl_stream<boost::beast::tcp_stream>& in_ssl_stream,
      std::shared_ptr<client> in_self_attr
  )
      : ssl_stream(in_ssl_stream),
        self_attr(std::move(in_self_attr)),
        p_data(std::make_shared<data_type>(std::move(in_req_attr), std::move(in_url_attr))),
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
    l_data->req_attr.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    l_data->req_attr.prepare_payload();
  }

  template <typename Self>
  void operator()(Self& self, boost::system::error_code error = {}, const Res& in_res = {}) {
    /// 检查错误
    using namespace std::literals;
    boost::beast::get_lowest_layer(ssl_stream).expires_after(30s);  /// 更新超时
    auto l_data = p_data;     /// 在这里复制一次内部数据, 放置移动时数据无法访问
    auto l_c    = self_attr;  /// 在这里复制一次客户端,放置客户端被移动时无法访问
    if (error) {
      boost::asio::post(l_c->get_executor(), [l_self = std::move(self), error, l_data]() mutable {
        l_self.complete(error, {});
      });
    }

    switch (state_) {
      case on_starting: {
        if (l_c->is_connect()) {
          state_ = on_handshake;  /// 如果已经连接,直接跳转到握手完成开始写入
          boost::asio::post(l_c->get_executor(), [l_self = std::move(self), error]() mutable {
            l_self(error);
          });  /// 直接开始下一步调用
        } else {
          state_ = on_resolve;  /// 开始进行ip解析已经各种证书验证, 和握手
          const std::string host{l_data->url_attr.host()};
          l_c->set_openssl(host);
          using namespace std::literals;
          const std::string port{
              l_data->url_attr.has_port()  //
                  ? std::string{l_data->url_attr.port()}
                  : "443"s};
          l_c->resolver().async_resolve(
              host, port,
              [self = std::move(self), l_c, l_data](
                  boost::system::error_code ec, const boost::asio::ip::tcp::resolver::results_type& results
              ) mutable {
                l_data->results_attr = results;
                self(ec, Res{});
              }
          );
        }
        break;
      }

      case on_resolve: {
        state_ = on_connected;
        ssl_stream.set_verify_mode(boost::asio::ssl::verify_peer);
        ssl_stream.set_verify_callback([](bool preverified, boost::asio::ssl::verify_context& ctx) -> bool {
          char subject_name[256];
          X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
          X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
          DOODLE_LOG_INFO("Verifying {}", subject_name);

          return true;
        });

        boost::beast::get_lowest_layer(ssl_stream)
            .async_connect(
                l_data->results_attr,
                [self = std::move(self
                 )](boost::system::error_code ec,
                    const boost::asio::ip::tcp::resolver::results_type::endpoint_type&) mutable { self(ec); }
            );
        break;
      }
      case on_connected: {
        state_ = on_handshake;
        ssl_stream.async_handshake(
            boost::asio::ssl::stream_base::client,
            [self = std::move(self)](boost::system::error_code ec) mutable { self(ec); }
        );
        break;
      }
      case on_handshake: {
        state_ = on_writing;
        l_c->is_connect(true);
        this->write_prepare();
        boost::beast::http::async_write(
            ssl_stream, l_data->req_attr,
            [l_self = std::move(self)](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
              boost::ignore_unused(bytes_transferred);
              l_self(ec);
            }
        );

        break;
      }

      case on_writing: {
        state_ = on_reading;
        boost::beast::http::async_read(
            ssl_stream, l_data->buffer_, l_data->res_attr,
            [l_self = std::move(self)](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
              boost::ignore_unused(bytes_transferred);
              l_self(ec);
            }
        );

        break;
      }
      case on_reading: {
        if (!l_data->res_attr.keep_alive()) self_attr->async_shutdown();
        boost::asio::post(l_c->get_executor(), [l_self = std::move(self), error, l_data]() mutable {
          l_self.complete(error, l_data->res_attr);
        });
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
class DOODLE_DINGDING_API client : public std::enable_shared_from_this<client> {
 public:
  using executor_type = typename boost::asio::any_io_executor;

 protected:
  [[nodiscard("")]] boost::beast::ssl_stream<boost::beast::tcp_stream>& ssl_stream();

  class impl;
  std::unique_ptr<impl> ptr;

 public:
  void set_openssl(const std::string& host);
  bool is_connect() const;
  void is_connect(bool in_connect);

  [[nodiscard("")]] boost::asio::ssl::context& ssl_context();
  [[nodiscard("")]] boost::asio::ip::tcp::resolver& resolver();

  void async_shutdown();

 public:
  explicit client(const boost::asio::any_io_executor& in_executor, boost::asio::ssl::context& in_ssl_context);

  executor_type get_executor() noexcept;

  template <typename Response, typename CompletionToken, typename Request>
  auto async_write_read(Request in_request, boost::url in_url, CompletionToken&& in_token)
      -> decltype(boost::asio::async_compose<
                  CompletionToken, void(boost::system::error_code, const std::decay_t<Response>&)>(
          std::declval<client_ns::async_http_req_res<Request, Response>>(), in_token, ssl_stream()
      )) {
    using http_req_res = client_ns::async_http_req_res<Request, Response>;
    return boost::asio::async_compose<CompletionToken, void(boost::system::error_code, const std::decay_t<Response>&)>(
        http_req_res{std::move(in_request), std::move(in_url), ssl_stream(), shared_from_this()}, in_token, ssl_stream()
    );
  };

 protected:
 public:
  virtual ~client() noexcept;
};

}  // namespace doodle::dingding
