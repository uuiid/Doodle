

#pragma once

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/json_rpc/core/rpc_server.h"
#include "doodle_core/metadata/metadata.h"

#include "doodle_lib/configure/doodle_lib_export.h"

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

#include <entt/entity/fwd.hpp>
#include <memory>
#include <string>
#include <vector>
#include <zmq.hpp>
#include <zmq_addon.hpp>

namespace doodle::distributed_computing {

class task : public doodle::json_rpc::rpc_server, public std::enable_shared_from_this<task> {
  /// 工作组
  std::shared_ptr<zmq::socket_t> socket_server;
  bool is_stop{};
  // boost::asio::strand<boost::asio::any_io_executor> strand{};

 public:
  task();
  ~task();

  void run_task();

 private:
  template <typename T>
  void register_fun_t2();

 protected:
  std::vector<std::tuple<database, doodle::user>> list_users();
  std::vector<std::tuple<database, doodle::work_task_info>> get_user_work_task_info(
      const entt::handle& in_tocken, const entt::handle& in_user
  );

  void connect();
};

class DOODLELIB_API server {
  /// 请求(异步)
  std::shared_ptr<zmq::socket_t> socket_backend;
  /// 后端(路由)
  std::shared_ptr<zmq::socket_t> socket_frontend;
  /// 工作组
  std::vector<std::shared_ptr<task>> socket_server_list;

 public:
  std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard;

  server();
  ~server();

  void run();

 private:
  void create_backend();
};

}  // namespace doodle::distributed_computing