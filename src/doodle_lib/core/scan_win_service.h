//
// Created by TD on 2023/12/26.
//

#pragma once

#include <doodle_lib/core/scan_assets/base.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>

namespace doodle {
class scan_win_service_t {
  using timer_t     = boost::asio::steady_timer;
  using timer_ptr_t = std::shared_ptr<timer_t>;

  using signal_t     = boost::asio::signal_set;
  using signal_ptr_t = std::shared_ptr<signal_t>;

  timer_ptr_t timer_;
  signal_ptr_t signal_;

  std::array<std::shared_ptr<doodle::details::scan_category_t>, 3> scan_categories_;
  std::vector<details::scan_category_t::project_root_t> project_roots_;
  std::vector<bool> scan_categories_is_scan_;
  std::map<uuid, entt::handle> handle_map_;
  std::map<FSys::path, entt::handle> path_map_;

  std::map<boost::uuids::uuid, doodle::details::scan_category_data_ptr> scan_data_map_;

  void scan();

  void begin_scan();
  void add_handle(const std::vector<doodle::details::scan_category_data_ptr>& in_data_vec);

  void on_timer(const boost::system::error_code& ec);

public:
  scan_win_service_t()  = default;
  ~scan_win_service_t() = default;

  void start();

  const std::map<boost::uuids::uuid, doodle::details::scan_category_data_ptr>& get_scan_data() const {
    return scan_data_map_;
  }

  template <typename CompletionHandler>
  auto async_scan_data(CompletionHandler&& handler) {
    auto l_init = [this](auto&& in_handle) {
      using handler_type = std::decay_t<decltype(in_handle)>;
      boost::asio::post(boost::asio::bind_executor(
        g_io_context(), [this, in_handle = std::make_shared<handler_type>(std::forward<decltype(in_handle)>(in_handle)
        )]() {
          (*in_handle)(boost::system::error_code(), scan_data_map_);
        }
      ));
    };
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, decltype(scan_data_map_))>(
      l_init, handler
    );
  }
};
} // namespace doodle