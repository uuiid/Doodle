#pragma once

#include "doodle_core/doodle_core_fwd.h"

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio/executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/core/noncopyable.hpp>

#include <memory>
#include <unordered_map>

namespace doodle::http {
// 一个为任务分配计算机的模块
class data_computers_socket_io_impl;
class computers_assign_task : public boost::noncopyable {
  computers_assign_task() = default;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_{boost::asio::make_strand(g_io_context())};
  std::unordered_map<uuid, std::weak_ptr<data_computers_socket_io_impl>> computer_map_;

  void clear_offline_computer();
  boost::asio::awaitable<void> run_next_task_impl(std::shared_ptr<data_computers_socket_io_impl> in_computer);

 public:
  ~computers_assign_task() = default;

  static computers_assign_task& get_instance();
  // 注册在线计算机
  boost::asio::awaitable<void> register_computer(std::shared_ptr<data_computers_socket_io_impl> in_computer);
  // 分发任务
  boost::asio::awaitable<void> assign_task(const server_task_info& in_task_info);
  // 让计算机运行下一个任务
  boost::asio::awaitable<void> run_next_task(uuid in_computer);
  boost::asio::awaitable<void> run_next_task();
};

}  // namespace doodle::http