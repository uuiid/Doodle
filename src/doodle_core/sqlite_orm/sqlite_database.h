//
// Created by TD on 24-9-12.
//

#pragma once

#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/scan_data_t.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/lockfree/spsc_queue.hpp>

#include <tl/expected.hpp>
namespace doodle {
class sqlite_database {
  using executor_type   = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;

  using strand_type     = boost::asio::strand<boost::asio::io_context::executor_type>;
  using strand_type_ptr = std::shared_ptr<strand_type>;

  using timer_type      = executor_type::as_default_on_t<boost::asio::steady_timer>;
  using timer_type_ptr  = std::shared_ptr<timer_type>;

  boost::lockfree::spsc_queue<scan_data_t::database_t> queue_scan_data_{1024};
  boost::lockfree::spsc_queue<project_helper::database_t> queue_project_helper_{1024};

  boost::lockfree::spsc_queue<std::int32_t> queue_scan_data_uuid_{1024};
  boost::lockfree::spsc_queue<std::int32_t> queue_project_helper_uuid_{1024};

  std::shared_ptr<void> storage_any_;
  strand_type_ptr strand_;
  timer_type_ptr timer_;
  boost::asio::awaitable<void> run_impl();
  boost::asio::awaitable<void> save();


  void set_path(const FSys::path& in_path);

 public:
  sqlite_database()  = default;
  ~sqlite_database() = default;

  void run();
  /**
   * 这回调函数用于加载数据库,  并且将数据库中的id分配到 sql_id 池中,  以便后续操作,
   * @warning 只有这里会分配id,  之后的操作不会分配, 只会查找id是否为 0 作为插入和更新的依据,
   * 并且在插入id的时候会自动更新为实际id
   * @param in_path 输入的数据库路径
   */
  void load(const FSys::path& in_path);

  void operator()(scan_data_t::database_t&& in_data) { queue_scan_data_.push(in_data); }
  void operator()(project_helper::database_t&& in_data) { queue_project_helper_.push(in_data); }
  template <typename T>
  std::vector<T> get_by_uuid(const uuid& in_uuid);

  template <typename T>
  boost::asio::awaitable<tl::expected<std::int64_t, std::string>> install(T in_data);
  /**
   * @warning 注意, 传入是必须排序id, 以符合插入和修正的方法
   * @tparam T 任意优化类别
   * @param in_data 传入的数据
   * @return 插入的id(不包含更新的id)
   */
  template <typename T>
  boost::asio::awaitable<tl::expected<std::vector<std::int64_t>, std::string>> install_range(std::vector<T> in_data);

  template <typename T>
  void destroy(const std::int32_t& in_uuid);

  template <>
  void destroy<scan_data_t>(const std::int32_t& in_uuid) {
    queue_scan_data_uuid_.push(in_uuid);
  }

  template <>
  void destroy<project_helper>(const std::int32_t& in_uuid) {
    queue_project_helper_uuid_.push(in_uuid);
  }
};
}  // namespace doodle