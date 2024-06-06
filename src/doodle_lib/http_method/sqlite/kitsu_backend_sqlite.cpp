#include "kitsu_backend_sqlite.h"

#include <boost/asio.hpp>
//
#include <doodle_core/metadata/attendance.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
namespace doodle::http {

class kitsu_backend_sqlite::kitsu_backend_sqlite_fun {
 public:
  using executor_type = boost::asio::any_io_executor;
  boost::asio::any_io_executor executor_;
  boost::asio::any_io_executor get_executor() const { return executor_; }

  template <typename T>
  struct sqlite_save_data {
    std::vector<T> save_list_{};
    std::vector<boost::uuids::uuid> destroy_list_{};
    std::map<entt::entity, boost::uuids::uuid> map_id_{};  // 用于关联

    void operator()(pooled_connection& in_db) const {
      auto l_indexs = T::filter_exist(in_db, save_list_);
      std::vector<T> l_insert_list{};
      std::vector<T> l_update_list{};
      for (auto i = 0; i < save_list_.size(); ++i) {
        if (l_indexs[i]) {
          l_update_list.emplace_back(save_list_[i]);
        } else {
          l_insert_list.emplace_back(save_list_[i]);
        }
      }
      if (!l_insert_list.empty()) T::insert(in_db, l_insert_list, map_id_);
      if (!l_update_list.empty()) T::update(in_db, l_update_list);
      if (!destroy_list_.empty()) T::delete_by_ids(in_db, destroy_list_);
    }

    void get_data(kitsu_backend_sqlite::observer_data<T>& in_data) {
      auto [l_save, l_del] = in_data.get_data();
      // ranges::v;
      for (auto i = 0; i < l_save.size(); ++i) {
        save_list_.emplace_back(g_reg()->get<T>(l_save[i]));
      }
      destroy_list_ = l_del;
      in_data.clear();
    }
  };

  template <>
  struct sqlite_save_data<user> {
    std::vector<user> save_list_{};
    std::vector<boost::uuids::uuid> destroy_list_{};
    std::map<entt::entity, boost::uuids::uuid> self_map_id_{};  // 用于关联

    void operator()(pooled_connection& in_db) const {
      auto l_indexs = user::filter_exist(in_db, save_list_);
      std::vector<user> l_insert_list{};
      std::vector<user> l_update_list{};
      for (auto i = 0; i < save_list_.size(); ++i) {
        if (l_indexs[i]) {
          l_update_list.emplace_back(save_list_[i]);
        } else {
          l_insert_list.emplace_back(save_list_[i]);
        }
      }
      if (!l_insert_list.empty()) user::insert(in_db, l_insert_list);
      if (!l_update_list.empty()) user::update(in_db, l_update_list);
      if (!destroy_list_.empty()) user::delete_by_ids(in_db, destroy_list_);
    }

    void get_data(kitsu_backend_sqlite::observer_data<user>& in_data) {
      auto [l_save, l_del] = in_data.get_data();
      // ranges::v;
      for (auto i = 0; i < l_save.size(); ++i) {
        save_list_.emplace_back(g_reg()->get<user>(l_save[i]));
        self_map_id_[l_save[i]] = save_list_.back().id_;
      }
      destroy_list_ = l_del;
      in_data.clear();
    }
  };

  std::shared_ptr<sqlite_save_data<user>> save_user_{};
  std::shared_ptr<sqlite_save_data<work_xlsx_task_info_block>> save_work_xlsx_task_info_block_{};

  kitsu_backend_sqlite_fun(

  )
      : executor_(g_thread().get_executor()),

        save_user_(std::make_shared<sqlite_save_data<user>>()),
        save_work_xlsx_task_info_block_(std::make_shared<sqlite_save_data<work_xlsx_task_info_block>>()) {}

  void get_data(kitsu_backend_sqlite& in_data) {
    save_user_->get_data(std::get<observer_data<user>>(in_data.observer_data_));
    save_work_xlsx_task_info_block_->get_data(std::get<observer_data<work_xlsx_task_info_block>>(in_data.observer_data_)
    );
    save_work_xlsx_task_info_block_->map_id_ = save_user_->self_map_id_;
  }

  void operator()() const {
    auto l_db_conn = g_pool_db().get_connection();
    l_db_conn.execute("PRAGMA foreign_keys = ON;");
    auto l_tx      = sqlpp::start_transaction(l_db_conn);
    (*save_user_)(l_db_conn);
    (*save_work_xlsx_task_info_block_)(l_db_conn);
    l_tx.commit();
  }
};

void kitsu_backend_sqlite::init(pooled_connection& in_conn) {
  // 先创建user表, 以便其他外键关联
  user::create_table(in_conn);
  work_xlsx_task_info_block::create_table(in_conn);
  attendance::create_table(in_conn);

  std::map<boost::uuids::uuid, entt::entity> l_map_id{};
  {  // 先创建user, 其他使用user做关联选择
    auto l_user = user::select_all(in_conn);
    std::vector<entt::entity> l_entities{l_user.size()};
    g_reg()->create(l_entities.begin(), l_entities.end());
    g_reg()->insert<user>(l_entities.begin(), l_entities.end(), l_user.begin());

    for (auto i = 0; i < l_user.size(); ++i) {
      l_map_id[l_user[i].id_] = l_entities[i];
    }
  }

  // 再选择work_xlsx_task_info_block
  {
    auto l_tasks = work_xlsx_task_info_block::select_all(in_conn, l_map_id);
    std::vector<entt::entity> l_entities{l_tasks.size()};
    g_reg()->create(l_entities.begin(), l_entities.end());
    g_reg()->insert<work_xlsx_task_info_block>(l_entities.begin(), l_entities.end(), l_tasks.begin());
  }

  // todo: 最后选择调休

  // 最后连接引用
  for (auto [e, l_b] : g_reg()->view<work_xlsx_task_info_block>().each()) {
    g_reg()->get<user>(l_b.user_refs_).task_block_[l_b.year_month_] = e;
  }

  connect(*g_reg());
}

void kitsu_backend_sqlite::connect(entt::registry& in_registry_ptr) {
  std::apply([&in_registry_ptr](auto&... in_data) { (in_data.connect(in_registry_ptr), ...); }, observer_data_);
}
void kitsu_backend_sqlite::disconnect() {
  std::apply([](auto&... in_data) { (in_data.disconnect(), ...); }, observer_data_);
}

void kitsu_backend_sqlite::clear() {
  std::apply([](auto&... in_data) { (in_data.clear(), ...); }, observer_data_);
}

void kitsu_backend_sqlite::run() {
  timer_ptr_  = std::make_shared<timer_t>(g_io_context());
  logger_ptr_ = g_logger_ctrl().make_log("kitsu_backend_sqlite");
  connect(*g_reg());
  begin_save();
}
void kitsu_backend_sqlite::begin_save() {
  timer_ptr_->expires_after(std::chrono::seconds(1));
  timer_ptr_->async_wait([this](const boost::system::error_code& in_ec) {
    if (in_ec) {
      logger_ptr_->log(log_loc(), level::warn, "begin_save error:{}", in_ec.message());
      return;
    }
    save();
    begin_save();
  });
}

void kitsu_backend_sqlite::save() {
  kitsu_backend_sqlite_fun l_save{};
  l_save.get_data(*this);
  boost::asio::post(std::move(l_save));
}
}  // namespace doodle::http