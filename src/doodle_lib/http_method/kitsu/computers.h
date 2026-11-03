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
class data_computers_socket_io_impl;
class computers_assign_task : public boost::noncopyable {
  boost::asio::strand<boost::asio::io_context::executor_type> strand_{boost::asio::make_strand(g_io_context())};
  std::unordered_map<uuid, std::weak_ptr<data_computers_socket_io_impl>> computer_map_;
  friend class data_computers_socket_io_impl;
  void clear_offline_computer();
  boost::asio::awaitable<void> run_next_task_impl(std::shared_ptr<data_computers_socket_io_impl> in_computer);

 public:
  computers_assign_task()  = default;
  ~computers_assign_task() = default;

  static computers_assign_task& get_instance();
  boost::asio::awaitable<void> register_computer(std::shared_ptr<data_computers_socket_io_impl> in_computer);
  void unregister_computer(const uuid& in_computer_id);
  boost::asio::awaitable<void> run_next_task();
};

}  // namespace doodle::http