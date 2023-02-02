#include "server.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/json_rpc/exception/json_rpc_error.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/user.h"
#include "doodle_core/metadata/work_task.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/asio.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/system/system_error.hpp>
#include <boost/uuid/uuid.hpp>

#include "distributed_computing/server.h"
#include <cstddef>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entity/registry.hpp>
#include <fmt/core.h>
#include <functional>
#include <memory>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <rttr/type.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <zmq.h>
#include <zmq.hpp>

namespace doodle::distributed_computing {

task::task() : socket_server() {}

void task::run_task() {
  socket_server = std::make_shared<zmq::socket_t>(g_reg()->ctx().emplace<zmq::context_t>(), zmq::socket_type::rep);
  // socket_server->bind("tcp://*:23333");
  socket_server->connect("tcp://127.0.0.1:23334");

  register_fun_t("list.user"s, [this]() -> std::vector<std::tuple<entt::entity, user>> { return this->list_users(); });
  register_fun_t(
      "set.user"s,
      [this](const database& in_tocken, const entt::entity& in_e, const user& in_user) -> database {
        return this->set_user(in_tocken.find_by_uuid(), in_e, in_user);
      }
  );
  register_fun_t("new.user", [this](const user& in_user) { return this->new_user(in_user); });

  register_fun_t("get.user", [this](const boost::uuids::uuid& in_uuid) { return this->get_user(in_uuid); });

  register_fun_t(
      "get.filter.work_task_info"s,
      [this](
          const database& in_tocken, const entt::entity& in_user
      ) -> std::vector<std::tuple<entt::entity, doodle::work_task_info>> {
        return this->get_user_work_task_info(in_tocken.find_by_uuid(), entt::handle{*g_reg(), in_user});
      }
  );
  register_fun_t(
      "set.work_task_info",
      [this](const database& in_tocken, const entt::entity& in_e, const work_task_info& in_work) {
        return this->set_user_work_task_info(in_tocken.find_by_uuid(), in_e, in_work);
      }
  );

  register_fun_t("delete.work_task_info", [this](const database& in_tocken, const entt::entity& in_e) {
    return this->delete_work_task_info(in_tocken.find_by_uuid(), in_e);
  });

  register_fun_t("destroy.entity"s, [this](const entt::entity& in_e) { return this->destroy_entity(in_e); });

  register_fun_t("rpc.list_fun"s, [this, self = weak_from_this()]() -> std::vector<std::string> {
    auto l_list = fun_list_ |
                  ranges::views::transform([](const std::pair<std::string, task::call_fun>& in) -> std::string {
                    return in.first;
                  }) |
                  ranges::to_vector;
    return l_list;
  });

  connect();
}

void task::connect() {
  boost::asio::post(g_io_context(), [this, self = shared_from_this()]() {
    zmq::message_t l_msg{};
    auto l_r = self->socket_server->recv(l_msg, zmq::recv_flags::none);
    // auto l_call_r = (*this)(l_msg.to_string());
    // l_msg.rebuild(l_call_r.data(), l_call_r.size());
    // self->socket_server->send(l_msg, zmq::send_flags::none);
    // if (!is_stop) connect();

    boost::asio::post(g_io_context(), [l_msg = std::move(l_msg), this, self = shared_from_this()]() mutable {
      auto l_call_r = (*self)(l_msg.to_string());
      l_msg.rebuild(l_call_r.data(), l_call_r.size());
      if (self->is_stop) return;

      boost::asio::post(g_io_context(), [l_msg = std::move(l_msg), this, self = shared_from_this()]() mutable {
        self->socket_server->send(l_msg, zmq::send_flags::none);
        if (!self->is_stop) connect();
      });
    });
  });
}

void task::close() {
  is_stop = true;
  socket_server->close();
}

std::vector<std::tuple<entt::entity, doodle::user>> task::list_users() {
  boost::ignore_unused(this);
  std::vector<std::tuple<entt::entity, doodle::user>> l_r{};
  for (auto&& [e, u] : g_reg()->view<user>().each()) {
    l_r.emplace_back(e, u);
  }
  return l_r;
}

database task::set_user(const entt::handle& in_tocken, const entt::entity& in_e, const user& in_user) {
  boost::ignore_unused(this);
  auto l_h = entt::handle{*g_reg(), in_e};
  if (!l_h) throw_exception(json_rpc::invalid_handle_exception{});
  /// 本身用户
  auto& l_tocken = in_tocken.get<user>();
  if (in_tocken == l_h) {
    auto l_user = in_user;
    if (l_tocken.power == power_enum::none) l_user.power = power_enum::none;
    l_h.emplace_or_replace<user>(l_user);
  } else {
    /// 检查授权
    if (l_tocken.power == power_enum::modify_other_users) {
      l_h.emplace_or_replace<user>(in_user);
    }
  }

  database::save(l_h);
  return l_h.get<database>();
}

std::tuple<entt::entity, database> task::new_user(const user& in) {
  boost::ignore_unused(this);
  auto l_h                               = entt::handle{*g_reg(), g_reg()->create()};
  l_h.emplace_or_replace<user>(in).power = power_enum::none;
  auto&& l_d                             = l_h.emplace<database>();
  database::save(l_h);
  return std::make_tuple(l_h.entity(), l_d);
}

std::tuple<entt::entity, user, database> task::get_user(const boost::uuids::uuid& in_uuid) {
  boost::ignore_unused(this);
  database l_d{in_uuid};
  auto l_h = l_d.find_by_uuid();
  if (!l_h) throw_exception(json_rpc::invalid_id_exception{});
  if (!l_h.all_of<user, database>()) throw_exception(json_rpc::missing_components_exception{});
  database::save(l_h);
  return std::make_tuple(l_h.entity(), l_h.get<user>(), l_h.get<database>());
};

std::vector<std::tuple<entt::entity, doodle::work_task_info>> task::get_user_work_task_info(
    const entt::handle& in_tocken, const entt::handle& in_user
) {
  boost::ignore_unused(this);
  std::vector<std::tuple<entt::entity, doodle::work_task_info>> l_r{};
  // 当没有权限或者只查询自身时, 直接返回错误
  if (in_tocken.get<user>().power != power_enum::modify_other_users && in_tocken != in_user) {
    return l_r;
  }

  for (auto&& [e, u] : g_reg()->view<work_task_info>().each()) {
    if (u.user_ref.user_attr() == in_user) l_r.emplace_back(e, u);
  }
  return l_r;
}

void task::set_user_work_task_info(
    const entt::handle& in_tocken, const entt::entity& in_, const work_task_info& in_work
) {
  boost::ignore_unused(this);
  auto l_ref_user = in_work.user_ref.user_attr();
  if (in_tocken.get<user>().power != power_enum::modify_other_users && in_tocken != l_ref_user) {
    throw_exception(json_rpc::insufficient_permissions_exception{});
  }

  auto l_h = entt::handle{*g_reg(), g_reg()->valid(in_) ? in_ : g_reg()->create(in_)};
  l_h.emplace_or_replace<work_task_info>(in_work).user_ref.user_attr(l_ref_user);
  if (!l_h.all_of<database>()) l_h.emplace<database>();
  database::save(l_h);
}

void task::delete_work_task_info(const entt::handle& in_tocken, const entt::entity& in_) {
  boost::ignore_unused(this);

  if (in_tocken.get<user>().power != power_enum::modify_other_users) {
    throw_exception(json_rpc::insufficient_permissions_exception{});
  }
  if (!g_reg()->valid(in_)) {
    throw_exception(json_rpc::invalid_id_exception{});
  }
  auto l_h = entt::handle{*g_reg(), in_};
  if (!l_h.all_of<work_task_info>()) {
    throw_exception(json_rpc::missing_components_exception{});
  }
  l_h.erase<work_task_info>();

  if (!l_h.all_of<database>()) l_h.emplace<database>();
  database::save(l_h);
}

void task::destroy_entity(const entt::entity& in_) {
  boost::ignore_unused(this);
  auto l_h = entt::handle{*g_reg(), in_};
  if (!l_h) {
    throw_exception(json_rpc::invalid_handle_exception{});
  }
  database::delete_(l_h);
}

task::~task() = default;

server::server() : socket_frontend(), socket_backend(), socket_server_list() {
  g_reg()->ctx().emplace<zmq::context_t>();
}

void server::run() {
  work_guard = std::make_shared<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
      boost::asio::make_work_guard(g_io_context())
  );
  boost::asio::post(g_thread(), [this]() {
    socket_frontend = std::make_shared<zmq::socket_t>(g_reg()->ctx().at<zmq::context_t>(), zmq::socket_type::router);
    socket_backend  = std::make_shared<zmq::socket_t>(g_reg()->ctx().at<zmq::context_t>(), zmq::socket_type::dealer);
    socket_frontend->bind("tcp://*:23333");
    socket_backend->bind("tcp://*:23334");
    zmq_proxy(socket_frontend->handle(), socket_backend->handle(), nullptr);
  });
  boost::asio::post(g_io_context(), [&]() { create_backend(); });
}

void server::create_backend() {
  auto&& l_task = socket_server_list.emplace_back(std::make_shared<task>());
  l_task->register_fun_t("rpc.close"s, [this, l_task]() {
    l_task->close();
    work_guard.reset();
  });
  l_task->run_task();
}
server::~server() = default;
}  // namespace doodle::distributed_computing