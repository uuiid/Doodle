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
  struct ue_data {
    ue_data() = default;
    inline ~ue_data() {
      if (run_handle) run_handle.destroy();
    }

    entt::entity server_id{entt::null};
    entt::handle run_handle{};
  };
  struct data_type {
    timer_ptr timer_{};
    signal_set_ptr signal_set_{};
    std::shared_ptr<client_core> core_ptr_;
    entt::entity computer_id{entt::null};

    udp_client_ptr udp_client_ptr_{};
    std::shared_ptr<ue_data> ue_data_ptr_{};
  };

  std::shared_ptr<data_type> ptr_;

 public:
  work() : ptr_{std::make_shared<data_type>()} { make_ptr(); }

  // copy
  work(const work&)            = default;
  work& operator=(const work&) = default;
  // move
  work(work&&)                 = default;
  work& operator=(work&&)      = default;

  bool find_server_address(std::uint16_t in_port = 50022);

  ~work() = default;

  void run();

  void send_server_state();
  void send_log(std::string in_log);
  void send_err(std::string in_err);

  void run_job(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap);

 private:
  void make_ptr();

  void do_register();

  void do_wait();
  void do_close();

  void on_wait(boost::system::error_code ec);
};
using work_ptr = std::shared_ptr<work>;

}  // namespace render_farm
}  // namespace doodle
