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

  std::shared_ptr<void> storage_any_;
  strand_type_ptr strand_;

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

  template <typename T>
  std::vector<T> get_by_uuid(const uuid& in_uuid);

  template <typename T>
  boost::asio::awaitable<tl::expected<void, std::string>> install(std::shared_ptr<T> in_data);
  /**
   *
   * @tparam T 任意优化类别
   * @param in_data 传入的数据
   * @return 插入的id(不包含更新的id)
   */
  template <typename T>
  boost::asio::awaitable<tl::expected<void, std::string>> install_range(std::shared_ptr<std::vector<T>> in_data);



};
}  // namespace doodle