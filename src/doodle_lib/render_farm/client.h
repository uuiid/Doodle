//
// Created by td_main on 2023/8/18.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
namespace doodle {
class client {
 private:
  using timer                  = boost::asio::system_timer;
  using timer_ptr              = std::shared_ptr<timer>;
  using socket                 = boost::beast::tcp_stream;
  using socket_ptr             = std::shared_ptr<socket>;
  using resolver               = boost::asio::ip::tcp::resolver;
  using resolver_ptr           = std::shared_ptr<resolver>;
  using buffer_type            = boost::beast::flat_buffer;
  using response_type          = boost::beast::http::response<render_farm::detail::basic_json_body>;
  using request_type           = boost::beast::http::request<render_farm::detail::basic_json_body>;
  using message_generator_type = boost::beast::http::message_generator;
  using message_generator_ptr  = std::shared_ptr<message_generator_type>;
  struct data_type {
    std::string server_ip_;

    socket_ptr socket_{};
    timer_ptr timer_{};
    resolver_ptr resolver_{};

    buffer_type buffer_;
    response_type response_;
    message_generator_ptr request_;
    std::int32_t connect_count_{};
  };
  std::shared_ptr<data_type> ptr_;

  void make_ptr();

 public:
  explicit client(std::string in_server_ip) : ptr_(std::make_shared<data_type>()) {
    ptr_->server_ip_ = std::move(in_server_ip);
    make_ptr();
  }
  ~client() = default;

  // run
  void run();

  // rest run
  void rest_run();

  // server_ip
  [[nodiscard]] inline std::string& server_ip() { return ptr_->server_ip_; }
  [[nodiscard]] inline const std::string& server_ip() const { return ptr_->server_ip_; }

  // stream

  template <typename ExecutorType, typename CompletionHandler>
  auto async_connect(const ExecutorType& in_executor_type, CompletionHandler&& in_completion) {
    //    using async_completion =
    //        boost::asio::async_completion<CompletionHandler, void(boost::system::error_code, socket_ptr)>;
    //    using handler_type = typename async_completion::completion_handler_type;
    using base_type = boost::beast::async_base<std::decay_t<CompletionHandler>, ExecutorType>;
    struct connect_op : base_type, boost::asio::coroutine {
      enum state {
        start,
        resolve,
        connect,
        write,
        read,
      };
      client* ptr_;
      state state_ = start;
      connect_op(client* in_ptr, CompletionHandler&& in_handler, const ExecutorType& in_executor_type_1)
          : base_type(std::move(in_handler), in_executor_type_1), ptr_(in_ptr) {
        if (ptr_->ptr_->socket_->socket().is_open()) {
          do_write();
        } else {
          do_resolve();
        }
      }

      //      void operator()(boost::system::error_code ec, socket_ptr in_ptr) {
      //        if (ptr_->ptr_->socket_->socket().is_open()) {
      //          do_write();
      //        } else {
      //          do_resolve();
      //        }
      //      }
      void operator()(boost::system::error_code ec, std::size_t bytes_transferred) {
        switch (state_) {
          case state::write: {
            boost::ignore_unused(bytes_transferred);
            if (ec == boost::beast::errc::not_connected || ec == boost::beast::errc::connection_reset ||
                ec == boost::beast::errc::connection_refused || ec == boost::beast::errc::connection_aborted) {
              DOODLE_LOG_INFO("失去连接, 开始重新连接");
              do_resolve();
              return;
            }
            do_read();
            break;
          }
          case state::read: {
            boost::ignore_unused(bytes_transferred);
            if (ec) {
              DOODLE_LOG_INFO("{}", ec.message());
              return;
            }
            this->complete(false, ec, ptr_->ptr_->socket_);
            break;
          }
        }
      }

      void operator()(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
        boost::asio::async_connect(ptr_->ptr_->socket_->socket(), results, std::move(*this));
      }
      void operator()(boost::system::error_code ec, boost::asio::ip::tcp::endpoint endpoint) {
        boost::ignore_unused(endpoint);
        if (ec) {
          DOODLE_LOG_INFO("{}", ec.message());
          return;
        }
        DOODLE_LOG_INFO("连接成功服务器");
        do_write();
      }

      void do_write() {
        state_ = write;
        boost::beast::http::request<boost::beast::http::empty_body> l_request{
            boost::beast::http::verb::get, "/v1/render_farm", 11};
        l_request.keep_alive(true);
        l_request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        ptr_->ptr_->request_ = std::make_shared<message_generator_type>(std::move(l_request));
        boost::beast::async_write(ptr_->ptr_->socket_->socket(), std::move(*ptr_->ptr_->request_), std::move(*this));
      }
      void do_read() {
        state_ = read;
        ptr_->ptr_->buffer_.clear();
        ptr_->ptr_->response_ = {};
        boost::beast::http::async_read(
            ptr_->ptr_->socket_->socket(), ptr_->ptr_->buffer_, ptr_->ptr_->response_, std::move(*this)
        );
      }
      void do_resolve() { ptr_->ptr_->resolver_->async_resolve(ptr_->ptr_->server_ip_, "50021", std::move(*this)); }
    };

    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, socket_ptr)>(
        [](auto&& in_completion_, client* in_client_ptr, const auto& in_executor_) {
          connect_op op{in_client_ptr, std::forward<decltype(in_completion_)>(in_completion_), in_executor_};
        },
        in_completion, this, in_executor_type
    );
  }

  void list_task();

  inline void server_ip(std::string in_server_ip) { ptr_->server_ip_ = std::move(in_server_ip); }

 private:
  void do_wait();
  void do_resolve();

  // hello world
  void do_hello();

  void do_write();
  void do_read();

  // 获取渲染注册机器列表
  void do_get_computer_list();
  // 获取所有的渲染任务
  void do_get_task_list();

  // on resolve
  void on_resolve(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results);

  // on_connect
  void on_connect(boost::system::error_code ec, boost::asio::ip::tcp::endpoint endpoint);
  // on_connect_timeout
  void on_connect_timeout(boost::system::error_code ec);

  void on_write(boost::system::error_code ec, std::size_t bytes_transferred);
  void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
};

}  // namespace doodle
