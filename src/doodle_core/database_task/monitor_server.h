//
// Created by TD on 2023/1/14.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>

#include <utility>
namespace doodle::database_n {

class monitor_server {
  registry_ptr reg{};

  entt::observer obs_create_database{};
  entt::observer obs_create_user{};
  entt::observer obs_create_work_task_info{};

  entt::observer obs_updata_user{};
  entt::observer obs_updata_work_task_info{};

  std::vector<std::int64_t> database_on_destroy_data{};
  void database_on_destroy(entt::registry& in_reg, entt::entity);

  void tick(const boost::system::error_code& in_code);
  void run_tick();

  void save();
  boost::asio::system_timer timer;

 public:
  template <
      typename T, typename std::enable_if_t<
                      boost::asio::is_executor<T>::value || boost::asio::execution::is_executor_v<T>>* = nullptr>
  explicit monitor_server(registry_ptr in_registry, const T& in_exe) : reg(std::move(in_registry)), timer(in_exe) {
    monitor();
  };
  template <
      typename T, typename std::enable_if_t<std::is_convertible_v<T&, boost::asio::execution_context&>>* = nullptr>
  explicit monitor_server(registry_ptr in_registry, T& in_exe)
      : reg(std::move(in_registry)), timer(boost::asio::make_strand(in_exe)) {
    monitor();
  };
  //  template <typename T>
  //  explicit monitor_server(registry_ptr in_registry, T&& in_exe)
  //      : reg(std::move(in_registry)), timer(boost::asio::make_strand(in_exe)) {
  //    monitor();
  //  };

  monitor_server() : monitor_server(g_reg(), g_io_context()){};

  void monitor();

  void load_all();
};

}  // namespace doodle::database_n
