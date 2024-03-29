//
// Created by TD on 2023/12/26.
//

#pragma once

#include <doodle_lib/core/scan_assets/base.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
namespace doodle {
class scan_win_service_t {
  using timer_t      = boost::asio::steady_timer;
  using timer_ptr_t  = std::shared_ptr<timer_t>;

  using signal_t     = boost::asio::signal_set;
  using signal_ptr_t = std::shared_ptr<signal_t>;

  timer_ptr_t timer_;
  signal_ptr_t signal_;

  std::vector<doodle::details::scan_category_data_ptr> scam_data_vec_;
  std::array<std::shared_ptr<doodle::details::scan_category_t>, 3> scan_categories_;
  std::vector<details::scan_category_t::project_root_t> project_roots_;
  std::vector<bool> scan_categories_is_scan_;
  std::map<uuid, entt::handle> handle_map_;
  std::map<FSys::path, entt::handle> path_map_;

  void scan();
  void open_project();

  void end_open_project();
  void add_handle(const std::vector<doodle::details::scan_category_data_ptr>& in_data_vec);

  void on_timer(const boost::system::error_code& ec);

 public:
  scan_win_service_t()  = default;
  ~scan_win_service_t() = default;

  void start();
};
}  // namespace doodle