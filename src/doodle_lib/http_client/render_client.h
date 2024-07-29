//
// Created by TD on 24-7-26.
//

#pragma once
#include <doodle_core/core/http_client_core.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::render_client {
class client {
 public:
  using https_client_core     = doodle::http::detail::http_client_data_base;
  using https_client_core_ptr = std::shared_ptr<https_client_core>;

  using timer_t = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>::as_default_on_t<boost::asio::steady_timer>;
  using timer_ptr_t = std::shared_ptr<timer_t>;

 private:
  timer_ptr_t timer_ptr_{};
  https_client_core_ptr http_client_core_ptr_{};

 public:
  client(const std::string& in_url) : http_client_core_ptr_{std::make_shared<https_client_core>(g_io_context())} {
    http_client_core_ptr_->init(in_url);
  }

  ~client() = default;

  boost::asio::awaitable<void> render(
      std::string in_name, FSys::path in_exe_path, std::vector<std::string> in_run_args
  );
  boost::asio::awaitable<std::vector<computer>> get_computers();
  boost::asio::awaitable<std::vector<server_task_info>> get_task();
  boost::asio::awaitable<std::string> get_logger(boost::uuids::uuid in_uuid, level::level_enum in_level);
};
}  // namespace doodle::render_client
