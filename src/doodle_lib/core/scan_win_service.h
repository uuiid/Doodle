//
// Created by TD on 2023/12/26.
//

#pragma once

#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/shot.h>

#include <doodle_lib/core/scan_assets/base.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
namespace doodle {
namespace scan {
struct scan_key_t {
  details::assets_type_enum dep_;
  season season_;
  episodes episodes_;
  shot shot_;
  project project_;
  std::string number_;
  std::string name_;
  std::string version_name_;
};

}  // namespace scan

class scan_win_service_t {
  using timer_t      = boost::asio::steady_timer;
  using timer_ptr_t  = std::shared_ptr<timer_t>;

  using signal_t     = boost::asio::signal_set;
  using signal_ptr_t = std::shared_ptr<signal_t>;

  timer_ptr_t timer_;
  signal_ptr_t signal_;
  boost::asio::any_io_executor executor_{};

  std::array<std::shared_ptr<doodle::details::scan_category_t>, 3> scan_categories_;
  std::vector<details::scan_category_t::project_root_t> project_roots_;
  std::vector<bool> scan_categories_is_scan_;
  std::map<uuid, entt::handle> handle_map_;
  std::map<FSys::path, entt::handle> path_map_;

  using scan_category_data_id_map = std::map<boost::uuids::uuid, doodle::details::scan_category_data_ptr>;
  std::array<scan_category_data_id_map, 2> scan_data_maps_;

  using scan_category_data_key_map = std::map<scan::scan_key_t, doodle::details::scan_category_data_ptr>;
  std::array<scan_category_data_key_map, 2> scan_data_key_maps_;

  std::atomic_int index_{};

  boost::asio::awaitable<void> begin_scan();
  void add_handle(
      const std::vector<doodle::details::scan_category_data_ptr>& in_data_vec, std::int32_t in_current_index
  );

 public:
  scan_win_service_t()  = default;
  ~scan_win_service_t() = default;

  void start();

  const scan_category_data_id_map& get_scan_data() const { return scan_data_maps_[index_]; }
  const scan_category_data_key_map& get_scan_data_key() const { return scan_data_key_maps_[index_]; }
};
}  // namespace doodle