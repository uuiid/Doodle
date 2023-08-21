//
// Created by td_main on 2023/8/21.
//
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
namespace doodle {
namespace render_farm {

class work {
 public:
  using timer        = boost::asio::system_timer;
  using timer_ptr    = std::shared_ptr<timer>;
  using socket       = boost::asio::ip::tcp::socket;
  using socket_ptr   = std::shared_ptr<socket>;
  using resolver     = boost::asio::ip::tcp::resolver;
  using resolver_ptr = std::shared_ptr<resolver>;

 private:
  struct data_type {
    std::string server_ip{};
    timer_ptr timer_{};
    socket_ptr socket_{};
    resolver_ptr resolver_{};
  };
  std::shared_ptr<data_type> ptr_;

 public:
  explicit work(std::string in_server_ip) : ptr_{std::make_shared<data_type>()} {
    ptr_->server_ip = std::move(in_server_ip);
  }

  ~work() = default;

  void run();

 private:
  void do_register();
};

}  // namespace render_farm
}  // namespace doodle
