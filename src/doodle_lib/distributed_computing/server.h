

#pragma once

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/json_rpc/core/rpc_server.h"
#include "doodle_core/lib_warp/json_warp.h"
#include "doodle_core/metadata/metadata.h"

#include "doodle_lib/configure/doodle_lib_export.h"

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/uuid/uuid.hpp>

#include <entt/entity/fwd.hpp>
#include <memory>
#include <string>
#include <tuple>
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
  void close();

 private:
  template <typename T>
  void register_fun_t2();

 protected:
  /**
   * @brief 列出所有的用户
   *
   * @return std::vector<std::tuple<entt::entity, doodle::user>> 返回实体和用户组件
   */
  std::vector<std::tuple<entt::entity, doodle::user>> list_users();
  /**
   * @brief 获取用户的任务列表
   *
   * @param in_tocken 传入的权限句柄
   * @param in_user 使用传入的用户过滤
   * @return std::vector<std::tuple<entt::entity, doodle::work_task_info>> 实体和任务列表
   */
  std::vector<std::tuple<entt::entity, doodle::work_task_info>> get_user_work_task_info(
      const entt::handle& in_tocken, const entt::handle& in_user
  );
  /**
   * @brief 设置user
   * @li 更新旧的用户
   *
   *
   * @param in_tocken 传入的的权限句柄(为空是只能创建新用户)
   * @param in_e  传入的实体
   * @param in_user 传入的用户信息
   * @return database 返回权限类
   */
  database set_user(const entt::handle& in_tocken, const entt::entity& in_e, const user& in_user);
  /**
   * @brief 创建新用户
   *
   * @param in_user 传入的用户数据
   * @return database
   */
  std::tuple<entt::entity, database> new_user(const user& in_user);

  /**
   * @brief 登录user
   *
   * @param in_uuid 传入的uuid帐号
   * @return std::tuple<entt::entity, user, database> 实体,用户,授权
   */
  std::tuple<entt::entity, user, database> get_user(const boost::uuids::uuid& in_uuid);

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