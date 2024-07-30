#pragma once

#include <doodle_lib_fwd.h>

#include <entt/entt.hpp>
namespace doodle::http {
class task_sqlite_server : public boost::noncopyable {
  using timer_t     = boost::asio::steady_timer;
  using timer_ptr_t = std::shared_ptr<timer_t>;
  timer_ptr_t timer_ptr_{};

  entt::observer obs_update_;
  entt::observer obs_create_;
  std::vector<boost::uuids::uuid> destroy_ids_{};
  entt::connection conn_{};

  logger_ptr logger_ptr_{};
  void on_destroy(entt::registry& in_reg, entt::entity in_entt);  //{ destroy_ids_.emplace_back(in_entt); }
  void connect(entt::registry& in_registry_ptr);
  void disconnect();
  void clear();
    /**
   *
   * @return 需要保存的数据, 需要删除的数据
   */
  std::tuple<std::vector<entt::entity>, std::vector<boost::uuids::uuid>> get_data() const;

  void begin_save();

  void save();
 public:
  void init(pooled_connection& in_conn);
  void run();
};
}  // namespace doodle::http