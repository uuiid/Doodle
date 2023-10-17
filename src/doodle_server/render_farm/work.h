//
// Created by td_main on 2023/8/21.
//
#include <doodle_core/configure/static_value.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <doodle_server/render_farm/client_core.h>
#include <doodle_server/render_farm/detail/basic_json_body.h>
#include <doodle_server/render_farm/websocket.h>

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
  using websocket_ptr   = std::shared_ptr<websocket>;

 private:
  struct ue_data {
    ue_data() = default;
    inline ~ue_data() {
      if (run_handle) run_handle.destroy();
    }

    entt::entity server_id{entt::null};
    entt::handle run_handle{};
  };
  using send_state = std::bitset<8>;
  enum class send_state_enum : std::uint64_t {
    udp_find,
    reg_computer,
    send_server_state,
    send_log,
    send_error,
  };

  struct data_type {
    timer_ptr timer_{};
    timer_ptr send_log_timer_{};
    timer_ptr send_error_timer_{};
    signal_set_ptr signal_set_{};
    std::shared_ptr<client_core> core_ptr_;
    entt::handle websocket_handle{};
    entt::entity computer_id{entt::null};

    std::shared_ptr<ue_data> ue_data_ptr_{};
    logger_ptr logger_{};
    // send_error send_log send_server_state reg_computer udp_find
    std::bitset<8> state_{};

    std::string log_cache_{};
    std::string err_cache_{};
    std::string server_address_{};
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

  void find_server_address(std::uint16_t in_port = doodle_config::udp_port);

  ~work() = default;

  void run(const std::string& in_server_address, std::uint16_t in_port = doodle_config::http_port);
  void stop();

  void send_server_state();
  void send_log(std::string in_log);
  void send_err(std::string in_err);

  boost::system::error_code run_job(const entt::handle& in_handle, const nlohmann::json& in_json);

 private:
  void make_ptr();
  void do_register();
  void do_wait();
  void do_close();
  void do_connect();

  void send_log_impl();
  void send_error_impl();
};
using work_ptr = std::shared_ptr<work>;

}  // namespace render_farm
}  // namespace doodle
