//
// Created by td_main on 2023/8/21.
//
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/render_farm/client_core.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace doodle {
namespace render_farm {

class work {
 public:
  using client_core     = doodle::detail::client_core;
  using client_core_ptr = std::shared_ptr<client_core>;
  //  boost::asio::basic_io_object;
  using timer           = boost::asio::high_resolution_timer;
  using timer_ptr       = std::shared_ptr<timer>;

  using response_type   = boost::beast::http::response<boost::beast::http::string_body>;
  using request_type    = boost::beast::http::request<boost::beast::http::string_body>;
  using signal_set      = boost::asio::signal_set;
  using signal_set_ptr  = std::shared_ptr<signal_set>;

 private:
  struct data_type {
    timer_ptr timer_{};
    signal_set_ptr signal_set_{};
    std::shared_ptr<client_core> core_ptr_;
  };
  std::shared_ptr<data_type> ptr_;

 public:
  explicit work(std::string in_server_ip) : ptr_{std::make_shared<data_type>()} {
    ptr_->core_ptr_ = std::make_shared<client_core>(std::move(in_server_ip));

    make_ptr();
  }

  ~work() = default;

  void run();

  void send_server_state(const entt::handle& in_handle);

 private:
  void make_ptr();

  void do_wait();
  void do_close();

  void on_wait(boost::system::error_code ec);

  struct reg_action {
    using response_type = boost::beast::http::response<render_farm::detail::basic_json_body>;
    using result_type   = std::string;
    client_core* ptr_;
    boost::beast::http::message_generator operator()();
    result_type operator()(const response_type& in_response);
  };
};

}  // namespace render_farm
}  // namespace doodle
