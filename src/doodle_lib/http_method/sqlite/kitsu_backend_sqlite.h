#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <entt/entt.hpp>

namespace doodle {
class user;
class work_xlsx_task_info_block;
class attendance_block;

}  // namespace doodle

namespace doodle::http {
class kitsu_backend_sqlite : public boost::noncopyable {
  using timer_t     = boost::asio::steady_timer;
  using timer_ptr_t = std::shared_ptr<timer_t>;
  timer_ptr_t timer_ptr_{};

  class kitsu_backend_sqlite_fun;

  template <typename T>
  struct observer_data {
    entt::observer obs_update_;
    entt::observer obs_create_;
    std::vector<boost::uuids::uuid> destroy_ids_{};
    entt::connection conn_{};

    ~observer_data() { disconnect(); }

    void on_destroy(entt::registry& in_reg, entt::entity in_entt) {
      destroy_ids_.emplace_back(in_reg.get<T>(in_entt).id_);
    }
    void connect(entt::registry& in_reg) {
      obs_update_.connect(in_reg, entt::collector.update<T>());
      obs_create_.connect(in_reg, entt::collector.group<T>());
      conn_ = in_reg.on_destroy<T>().connect<&observer_data::on_destroy>(*this);
    }

    void disconnect() {
      obs_update_.disconnect();
      obs_create_.disconnect();
      if (conn_) conn_.release();
    }

    void clear() {
      obs_update_.clear();
      obs_create_.clear();
      destroy_ids_.clear();
    }

    // has value
    operator bool() const { return !obs_update_.empty() || !obs_create_.empty() || !destroy_ids_.empty(); }

    std::tuple<std::vector<entt::entity>, std::vector<boost::uuids::uuid>> get_data() const {
      std::vector<entt::entity> l_data{};
      for (const auto& entity : obs_update_) {
        l_data.push_back(entity);
      }
      for (const auto& entity : obs_create_) {
        l_data.push_back(entity);
      }
      l_data |= ranges::actions::sort;
      l_data |= ranges::actions::unique;
      auto l_destroy_ids = destroy_ids_;
      l_destroy_ids |= ranges::actions::sort;
      l_destroy_ids |= ranges::actions::unique;
      return {std::move(l_data), l_destroy_ids};
    }
  };

  std::tuple<observer_data<user>, observer_data<work_xlsx_task_info_block>, observer_data<attendance_block>>
      observer_data_{};

  logger_ptr logger_ptr_{};

  void connect(entt::registry& in_registry_ptr);
  void disconnect();
  void clear();

  void begin_save();

  void save();

 public:
  void init(pooled_connection& in_conn);
  void run();
};
}  // namespace doodle::http