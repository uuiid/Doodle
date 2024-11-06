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
#include <boost/describe/class.hpp>
#include <boost/describe/operators.hpp>

namespace doodle::scan {
struct scan_key_t : boost::totally_ordered<scan_key_t> {
  details::assets_type_enum dep_;
  season season_;
  episodes episodes_;
  shot shot_;
  /// 特别指定为 kitsu_uuid_
  uuid project_;
  std::string number_;
  std::string name_;
  std::string version_name_;

  friend bool operator==(const scan_key_t& lhs, const scan_key_t& rhs) {
    return std::tie(
               lhs.dep_, lhs.season_, lhs.episodes_, lhs.shot_, lhs.project_, lhs.number_, lhs.name_, lhs.version_name_
           ) ==
           std::tie(
               rhs.dep_, rhs.season_, rhs.episodes_, rhs.shot_, rhs.project_, rhs.number_, rhs.name_, rhs.version_name_
           );
  }
  friend bool operator<(const scan_key_t& lhs, const scan_key_t& rhs) {
    return std::tie(
               lhs.dep_, lhs.season_, lhs.episodes_, lhs.shot_, lhs.project_, lhs.number_, lhs.name_, lhs.version_name_
           ) <
           std::tie(
               rhs.dep_, rhs.season_, rhs.episodes_, rhs.shot_, rhs.project_, rhs.number_, rhs.name_, rhs.version_name_
           );
  }
};
// template <>
// struct boost::totally_ordered<scan_key_t>;
}  // namespace doodle::scan

namespace std {
template <>
struct hash<doodle::scan::scan_key_t> {
  std::size_t operator()(const doodle::scan::scan_key_t& value) const noexcept {
    std::size_t seed = 0;

    boost::hash_combine(seed, value.dep_);
    boost::hash_combine(seed, value.season_);
    boost::hash_combine(seed, value.episodes_);
    boost::hash_combine(seed, value.shot_);
    boost::hash_combine(seed, value.project_);
    boost::hash_combine(seed, value.number_);
    boost::hash_combine(seed, value.name_);
    boost::hash_combine(seed, value.version_name_);
    return seed;
  }
};
}  // namespace std
namespace doodle {

class scan_win_service_t {
  using timer_t      = boost::asio::steady_timer;
  using timer_ptr_t  = std::shared_ptr<timer_t>;

  using signal_t     = boost::asio::signal_set;
  using signal_ptr_t = std::shared_ptr<signal_t>;

  static constexpr std::string_view jaon_file_name_{"scan_win_service.json"};

  timer_ptr_t timer_;
  signal_ptr_t signal_;
  boost::asio::any_io_executor executor_{};
  boost::asio::thread_pool thread_pool_{};

  logger_ptr logger_;

  std::array<std::shared_ptr<doodle::details::scan_category_t>, 3> scan_categories_;
  std::vector<std::shared_ptr<project_helper::database_t>> project_roots_;

  using scan_category_data_id_map = std::map<boost::uuids::uuid, doodle::details::scan_category_data_ptr>;
  std::array<scan_category_data_id_map, 2> scan_data_maps_;

  using scan_category_data_key_map = std::map<scan::scan_key_t, doodle::details::scan_category_data_ptr>;
  std::array<scan_category_data_key_map, 2> scan_data_key_maps_;

  std::atomic_int index_{};

  boost::asio::awaitable<void> begin_scan();
  void create_project();
  void seed_to_sql(std::int32_t in_current_index);

  void add_handle(
      const std::vector<doodle::details::scan_category_data_ptr>& in_data_vec, std::int32_t in_current_index
  );

  void init_all_map();
  bool use_cache_{};

 public:
  scan_win_service_t()  = default;
  ~scan_win_service_t() = default;

  void start();
  void use_cache(bool in_cache = true) { use_cache_ = in_cache; };

  const scan_category_data_id_map& get_scan_data() const { return scan_data_maps_[index_]; }
  const scan_category_data_key_map& get_scan_data_key() const { return scan_data_key_maps_[index_]; }
};
}  // namespace doodle
